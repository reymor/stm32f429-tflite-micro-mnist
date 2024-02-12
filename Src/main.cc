/*
 * main.cc
 *
 *  Created on: 14-Nov-2020
 *      Author: reymor
 *
 *  Brief: Main Functions
 */

/* Start of includes */
#include <stdio.h>
#include "main.h"
#include "debug.h"
#include "menu_images.h"
/* End of include */

/* Start of Tiny ML includes */
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include <model_int8.h> // Model
/* End of Tiny ML includes */

/* Private defines */
#define CIRCLE_PENCIL		4U
#define WORKING_HEIGHT 		160U
#define WORKING_WIDTH  		160U
#define TRANSFORMED_HEIGHT  28U
#define TRANSFORMED_WIDTH	28U

#define kNumberOfOutputs 	10U

/* Private global variables */
static uint8_t _run_model = 0;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void draw_menu(void);
static void check_touch(void);
static void update_color(void);
static void prepare_working_window(void);
static void rgb2gray(void);
static void resize_bilnear(void);
static void update_tensor_input(TfLiteTensor * in);
static uint8_t get_top_prediction(const int8_t* predictions, uint8_t num_categories);
static void print_result(uint8_t number, uint32_t tim);

int main(void)
{
	/* Hal Init */
	HAL_Init();

	/* Configure the system clock to 168 MHz */
	SystemClock_Config();

	/* Configure debug port */
	debug_init();

	/* Sending msg by uart */
	printf("Tiny ML - Handwritten number Recognition!\n");

	/* Configure LED3 and LED4 */
	BSP_LED_Init(LED3);
	BSP_LED_Init(LED4);

	/* --- Start of LCD Initialization --- */

	/* Initialize the LCD */
	BSP_LCD_Init();

	/* Layer2 Init */
	BSP_LCD_LayerDefaultInit(1, LCD_FRAME_BUFFER_LAYER1);
	/* Set Foreground Layer */
	BSP_LCD_SelectLayer(1);
	/* Clear the LCD */
	BSP_LCD_Clear(LCD_COLOR_WHITE);
	BSP_LCD_SetColorKeying(1, LCD_COLOR_WHITE);
	BSP_LCD_SetLayerVisible(1, DISABLE);

	/* Layer1 Init */
	BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER_LAYER0);

	/* Set Foreground Layer */
	BSP_LCD_SelectLayer(0);

	/* Enable The LCD */
	BSP_LCD_DisplayOn();

	/* Clear the LCD */
	BSP_LCD_Clear(LCD_COLOR_WHITE);

	/* --- End of LCD Initialization --- */

	/* --- Touch screen initialization --- */
	Touchscreen_Calibration();

	BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());

	/* Draw the menu */
	draw_menu();

	/* Start of TinyML Initialization */
	/* Set Up Logging */
	tflite::MicroErrorReporter micro_error_reporter;
	tflite::ErrorReporter * error_reporter = &micro_error_reporter;

	TfLiteTensor * input = nullptr;
	TfLiteTensor * output = nullptr;

	/* Loading the model */
	const tflite::Model * model  = tflite::GetModel(model_int8_tflite);
	if(model->version() != TFLITE_SCHEMA_VERSION)
	{
		error_reporter->Report("Model provided is schema version %d not equal"
							  "to supported version %d.\n",
							  model->version(), TFLITE_SCHEMA_VERSION);
		return 1;
	}

	static tflite::MicroMutableOpResolver<4> micro_op_resolver;
	micro_op_resolver.AddConv2D();
	micro_op_resolver.AddMaxPool2D();
	micro_op_resolver.AddFullyConnected();
	micro_op_resolver.AddReshape();

	const int tensor_arena_size = 30*1024;
	static uint8_t tensor_arena[tensor_arena_size];

	static tflite::MicroInterpreter static_interpreter(model, micro_op_resolver, tensor_arena, tensor_arena_size);

	TfLiteStatus allocate_status = static_interpreter.AllocateTensors();
	if( allocate_status != kTfLiteOk)
	{
		TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensor() failed");
		return 1;
	}

	input = static_interpreter.input(0);
	output = static_interpreter.output(0);

	/* --- End of Tiny-ML Initialization --- */

	for(;;)
	{
		check_touch(); /* check if the touch was touched to draw or select something in the menu */
		if(_run_model)
		{
			_run_model = 0; //clear the flag
			/* save the image from working window  160x160 image */
			prepare_working_window();
			/* convert image from rgb to gray */
			rgb2gray();
			/* resize the image to 28x28 to be used in the model */
			resize_bilnear();
			/* data from buffer to tensor input */
			update_tensor_input(input);

			/* invoke interpreter and print the results */
			uint32_t initial = HAL_GetTick();
			TfLiteStatus invoke_status = static_interpreter.Invoke();
			if( invoke_status != kTfLiteOk)
			{
				TF_LITE_REPORT_ERROR(error_reporter, "Invoke() failed");
				return 1;
			}
			uint32_t current = HAL_GetTick();

			int8_t result = get_top_prediction(output->data.int8, kNumberOfOutputs);

			print_result(result, current - initial);

			BSP_LED_Off(LED4);
		}
	}
}


static uint8_t get_top_prediction(const int8_t* predictions, uint8_t num_categories) {
  int8_t max_score = predictions[0];
  uint8_t guess = 0;

  for (int category_index = 1; category_index < num_categories;
       category_index++) {
    const int8_t category_score = predictions[category_index];
    if (category_score > max_score) {
      max_score = category_score;
      guess = category_index;
    }
  }

  return guess;
}

/* print the results on the screen */
static void print_result(uint8_t number, uint32_t time)
{
	static uint8_t op_buffer[30];
	static uint32_t color = BSP_LCD_GetTextColor(); // saves the color in the pen

	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_FillRect(67, (BSP_LCD_GetYSize()-90), 150, 70); // Clear the result windows if there are something
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK); //The result will be print with black

	snprintf((char *)op_buffer, 30, (char *)"The number is: %u", number);
	BSP_LCD_DisplayStringAt(67, (BSP_LCD_GetYSize()-90),(uint8_t*)op_buffer, LEFT_MODE);

	snprintf((char *)op_buffer, 30, (char *)"Time: %lu ms", time);
	BSP_LCD_DisplayStringAt(67, (BSP_LCD_GetYSize()-70), (uint8_t *)op_buffer, LEFT_MODE);
	BSP_LCD_SetTextColor(color); // back the color to the pen
}

/* copy the image to the input tensor */
static void update_tensor_input(TfLiteTensor * in)
{
	uint16_t idx;
	uint8_t * src = (uint8_t*) TRANSFORMED_FRAME_BUFFER;
	for(idx = 0; idx < in->bytes; idx++)
		in->data.int8[idx] = (int8_t)(src[idx] - 127); // the input is int8 and the image is in uint8 format
}

static void check_touch(void)
{
  uint32_t x = 0, y = 0, color;
  TS_StateTypeDef TS_State;

  /* Get Touch screen position */
  BSP_TS_GetState(&TS_State);

  /* Read the coordinate */
  x = Calibration_GetX(TS_State.X);
  y = Calibration_GetX(TS_State.Y);

  if ((TS_State.TouchDetected) && (x > 5) && (x < 55))
  {
    /* User selects one of the color pens */
    if ((y > 45) && (y < 85))
    {
      BSP_LCD_SetTextColor(LCD_COLOR_RED);
    }
    else if ((y > 90 ) && (y < (130)))
    {
      BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
    }
    else if ((y > (135)) && (y < (175)))
    {
      BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    }
    else if ((y > (220)) && (y < (270)))
    {
      /* Clear screen */
      /* Get the current text color */
      color = BSP_LCD_GetTextColor();
      BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
      /* Clear the result window */
      BSP_LCD_FillRect(67, (BSP_LCD_GetYSize() - 90), 150, 70);
      /* Clear the working window */
      BSP_LCD_FillRect(68, 8, 159, 159);
      BSP_LCD_SetTextColor(color);
    }
    else if ((y > (275)) && (y < (320)))
    {
    	BSP_LED_On(LED4);
    	_run_model = 1;
    }
    else
    {
      x = 0;
      y = 0;
    }
    update_color();
  }
  else if ((TS_State.TouchDetected) && (x > (67 + CIRCLE_PENCIL)) && (y > (7 + CIRCLE_PENCIL)) &&
		  (x < (BSP_LCD_GetXSize() - (7  + CIRCLE_PENCIL))) && (y < (BSP_LCD_GetYSize() - (155 + CIRCLE_PENCIL))))
  {
    BSP_LCD_FillCircle(x, y, CIRCLE_PENCIL); /*here its where you touched */
  }
}

void draw_menu(void)
{
	/* Set background Layer */
	BSP_LCD_SelectLayer(0);

	/* Clear the LCD */
	BSP_LCD_Clear(LCD_COLOR_WHITE);

	/* Draw clean image */
	BSP_LCD_DrawBitmap(0, BSP_LCD_GetYSize()-100, (uint8_t *)clean_bmp);

	/* Draw run_model image */
	BSP_LCD_DrawBitmap(0, BSP_LCD_GetYSize()-50, (uint8_t *)run_model_bmp);

	/* Draw colors image */
	BSP_LCD_DrawBitmap(2, BSP_LCD_GetYSize()-280, (uint8_t *)colors_bmp);

	/* Set Black as text color */
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

	/* Draw working window */
	BSP_LCD_DrawRect(61, 0, (BSP_LCD_GetXSize()-65), (BSP_LCD_GetYSize()-145));
	BSP_LCD_DrawRect(63, 3, (BSP_LCD_GetXSize()-70), (BSP_LCD_GetYSize()-150));
	BSP_LCD_DrawRect(65, 5, (BSP_LCD_GetXSize()-75), (BSP_LCD_GetYSize()-155));

	/* Draw result window */
	BSP_LCD_SetTextColor(LCD_COLOR_DARKGREEN);
	BSP_LCD_DrawRect(61, 186, (BSP_LCD_GetXSize()-65), (BSP_LCD_GetYSize()-200));
	BSP_LCD_SetFont(&Font16);
	BSP_LCD_SetTextColor(LCD_COLOR_RED);
	BSP_LCD_DisplayStringAt(30, (BSP_LCD_GetYSize()-120), (uint8_t *)"-- Results --", CENTER_MODE);
	BSP_LCD_SetFont(&Font12);

	/* Draw color indicator */
	BSP_LCD_SetTextColor(LCD_COLOR_DARKRED);
	BSP_LCD_SetFont(&Font12);
	BSP_LCD_DisplayStringAt(12, (BSP_LCD_GetYSize()-315), (uint8_t *)"Color", LEFT_MODE);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_DrawCircle(25, (BSP_LCD_GetYSize() - 295), CIRCLE_PENCIL);
	BSP_LCD_SetTextColor(LCD_COLOR_RED); // Default Color
	BSP_LCD_FillCircle(25, (BSP_LCD_GetYSize()-295), CIRCLE_PENCIL);
}

/* update the pencil color in the menu */
static void update_color(void)
{
  uint32_t color;

  /* Clear the current circle */
  color = BSP_LCD_GetTextColor();
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_FillCircle(25, (BSP_LCD_GetYSize()-295), CIRCLE_PENCIL);
  BSP_LCD_SetTextColor(color);

  /* Update the selected color icon */
  BSP_LCD_FillCircle(25, (BSP_LCD_GetYSize()-295), CIRCLE_PENCIL);

  /* Draw black circle */
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_DrawCircle(25, (BSP_LCD_GetYSize() - 295), CIRCLE_PENCIL);
  BSP_LCD_SetTextColor(color);
}

/* rgb to gray scale using CIE formula */
static void rgb2gray(void)
{
	uint8_t i, j;
	uint8_t *working_ptr = (uint8_t *) WORKING_FRAME_BUFFER;
	uint8_t *gray_ptr = (uint8_t *) GRAY_WORKING_FRAME_BUFFER;
	float l, r, g , b;
	uint32_t offset;

	for(i = 0; i < WORKING_WIDTH; i++)
	{
		for(j = 0; j < WORKING_HEIGHT; j++)
		{
			offset = i * WORKING_WIDTH + j;
			r = (float)(*(&working_ptr[offset*3]));
			g = (float)(*(&working_ptr[offset*3+1]));
			b = (float)(*(&working_ptr[offset*3+2]));
			//	 L  =  0.2126 × R   +   0.7152 × G   +   0.0722 × B  by CIE
			l = 0.2126*r + 0.7152*g + 0.0722*b;
			gray_ptr[offset] = (uint8_t)(255-l); // invert gray color because train images has
		}
	}

}

/* resize the image in gray color using bilinear method */
static void resize_bilnear(void)
{
	uint8_t * working_ptr 		= (uint8_t *)GRAY_WORKING_FRAME_BUFFER;
	uint8_t * transformed_ptr	= (uint8_t*)TRANSFORMED_FRAME_BUFFER;

	float x_ratio = ((float)(WORKING_WIDTH - 1))/TRANSFORMED_WIDTH;
	float y_ratio = ((float)(WORKING_HEIGHT -1))/TRANSFORMED_HEIGHT;
	float x_diff, y_diff;
	uint32_t A, B, C, D, x, y, index, gray;
	uint32_t offset = 0;
	uint32_t i, j;
	for(i = 0; i < TRANSFORMED_HEIGHT; i++)
	{
		for(j = 0; j < TRANSFORMED_WIDTH; j++)
		{
			x = (int)(x_ratio * j);
			y = (int)(y_ratio * i);
			x_diff = (x_ratio * j) - x;
			y_diff = (y_ratio * i) - y;
			index = y*WORKING_WIDTH + x;

			A = working_ptr[index];
			B = working_ptr[index + 1];
			C = working_ptr[index + WORKING_WIDTH];
			D = working_ptr[index + WORKING_WIDTH + 1];

		    gray = (int)(A*(1-x_diff)*(1-y_diff) +  B*(x_diff)*(1-y_diff) +
		    		   	   	C*(y_diff)*(1-x_diff)   +  D*(x_diff*y_diff));

		    transformed_ptr[offset++] = gray;
		}
	}
}

/* Save the working window where was drawn the number */
static void prepare_working_window(void)
{
  static DMA2D_HandleTypeDef hdma2d_dk;
  uint32_t address1 = WORKING_FRAME_BUFFER;
  uint32_t address2 = LCD_FRAME_BUFFER_LAYER0;
  uint32_t index = 0;

  memset((void*)WORKING_FRAME_BUFFER, 0xff, (size_t)380*240*4);

  /* Configure the DMA2D Mode, Color Mode and output offset */
  hdma2d_dk.Init.Mode         = DMA2D_M2M_PFC;
  hdma2d_dk.Init.ColorMode    = DMA2D_RGB888;
  hdma2d_dk.Init.OutputOffset = 0;

  /* Foreground Configuration */
  hdma2d_dk.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d_dk.LayerCfg[1].InputAlpha = 0xFF;
  hdma2d_dk.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  hdma2d_dk.LayerCfg[1].InputOffset = 60;

  hdma2d_dk.Instance = DMA2D;

  /* Bypass the bitmap header */
  address2 += ((BSP_LCD_GetXSize() * (BSP_LCD_GetYSize()-154) + 67) * 4);

  /* Addressing problem with image rotate */
  address1 += 160*160*3;

  /* Convert picture to RGB888 pixel format */
  for(index=0; index < (BSP_LCD_GetYSize() - 160); index++)
  {
    /* DMA2D Initialization */
    if(HAL_DMA2D_Init(&hdma2d_dk) == HAL_OK)
    {
      if(HAL_DMA2D_ConfigLayer(&hdma2d_dk, 1) == HAL_OK)
      {
        if (HAL_DMA2D_Start(&hdma2d_dk, address2, address1, (BSP_LCD_GetXSize() - 80), 1) == HAL_OK)
        {
          /* Polling For DMA transfer */
          HAL_DMA2D_PollForTransfer(&hdma2d_dk, 10);
        }
      }
    }
    /* Decrement the source and destination buffers */
    address1 -= ((BSP_LCD_GetXSize() - 80)*3);
    address2 -= BSP_LCD_GetXSize()*4;
  }
}

static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}


extern "C" void Error_Handler(void)
{
  /* Turn LED4 on */
  BSP_LED_On(LED4);
  while(1)
  {
  }
}
