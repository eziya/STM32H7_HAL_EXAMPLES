/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ECHO_SERVER_PORT    7
#define RECV_BUF_SIZE       1024
#define MAX_CLIENT_CONNECTIONS 3
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
osThreadId_t netTaskHandle;
const osThreadAttr_t netTask_attributes =
{ .name = "netTask", .stack_size = 1024 * 4, .priority = (osPriority_t) osPriorityNormal, };

void StartNetTask(void *argument)
{
  int server_sock;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_addr_len;
  char recv_buf[RECV_BUF_SIZE];
  const int on = 1;

  fd_set read_fds;
  int max_sd;
  int client_sockets[MAX_CLIENT_CONNECTIONS]; // 클라이언트 소켓 저장 배열
  int i, sd;

  // 클라이언트 소켓 배열 초기화
  for(i = 0;i < MAX_CLIENT_CONNECTIONS;i++)
  {
    client_sockets[i] = 0;
  }

  // 1. 서버 소켓 생성
  server_sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
  if(server_sock < 0)
  {
    vTaskDelete(NULL);
    return;
  }

  // SO_REUSEADDR 설정
  if(lwip_setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
  {
    lwip_close(server_sock);
    vTaskDelete(NULL);
    return;
  }

  // 2. 서버 주소 설정 및 바인딩
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = lwip_htons(ECHO_SERVER_PORT);
  server_addr.sin_addr.s_addr = lwip_htonl(INADDR_ANY);

  if(lwip_bind(server_sock, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
  {
    lwip_close(server_sock);
    vTaskDelete(NULL);
    return;
  }

  // 3. 연결 대기 (Listen)
  if(lwip_listen(server_sock, 5) < 0)
  {
    lwip_close(server_sock);
    vTaskDelete(NULL);
    return;
  }

  // 메인 select 루프
  while(1)
  {
    FD_ZERO(&read_fds);

    FD_SET(server_sock, &read_fds);
    max_sd = server_sock;

    for(i = 0;i < MAX_CLIENT_CONNECTIONS;i++)
    {
      sd = client_sockets[i];
      if(sd > 0)
      {
        FD_SET(sd, &read_fds);
      }

      if(sd > max_sd)
      {
        max_sd = sd;
      }
    }

    // select 호출: 이벤트 발생 대기 (NULL, NULL)은 타임아웃 없음.
    int activity = lwip_select(max_sd + 1, &read_fds, NULL, NULL, NULL);

    if((activity < 0) && (errno != EINTR))
    {
      // printf("select error: %d\r\n", errno);
      vTaskDelay(pdMS_TO_TICKS(100)); // 짧은 딜레이 후 재시도
      continue;
    }

    // 4. 서버 소켓에 새로운 연결 요청이 왔는지 확인
    if(FD_ISSET(server_sock, &read_fds))
    {
      client_addr_len = sizeof(client_addr);
      int new_sock = lwip_accept(server_sock, (struct sockaddr*) &client_addr, &client_addr_len);
      if(new_sock < 0)
      {
        // printf("accept error: %d\r\n", new_sock);
      }
      else
      {
        // 새 클라이언트 연결 수락
        // printf("New connection, socket fd is %d\r\n", new_sock);
        // char *ip_str = ip4addr_ntoa((const ip4_addr_t*)&client_addr.sin_addr);
        // if (ip_str != NULL) {
        //     printf("Client connected from %s:%d\r\n", ip_str, lwip_ntohs(client_addr.sin_port));
        // }

        for(i = 0;i < MAX_CLIENT_CONNECTIONS;i++)
        {
          if(client_sockets[i] == 0)
          {
            client_sockets[i] = new_sock;
            break;
          }
        }
        if(i == MAX_CLIENT_CONNECTIONS)
        {
          // printf("Max clients reached, closing new connection\r\n");
          lwip_close(new_sock);
        }
      }
    }

    // 5. 각 클라이언트 소켓에서 데이터가 왔는지 확인
    for(i = 0;i < MAX_CLIENT_CONNECTIONS;i++)
    {
      sd = client_sockets[i];

      if(sd > 0 && FD_ISSET(sd, &read_fds))
      {
        int recv_len = lwip_recv(sd, recv_buf, RECV_BUF_SIZE, 0);

        if(recv_len == 0)
        {
          // 클라이언트 연결 종료
          // printf("Host disconnected, fd %d\r\n", sd);
          lwip_close(sd);
          client_sockets[i] = 0; // 해당 슬롯 비움
        }
        else if(recv_len < 0)
        {
          // 에러 발생 (수신 에러)
          // printf("Receive error on fd %d: %d\r\n", sd, recv_len);
          lwip_close(sd);
          client_sockets[i] = 0; // 해당 슬롯 비움
        }
        else
        {
          // 데이터 수신 성공, 에코 처리
          lwip_send(sd, recv_buf, recv_len, 0);
        }
      }
    }
  }

  // 이 코드는 일반적으로 도달하지 않음
  lwip_close(server_sock);
  vTaskDelete(NULL);
}
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
  /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
  // 스택 오버플로우가 감지되면 시스템의 안정성을 위해 인터럽트를 비활성화합니다.
  taskDISABLE_INTERRUPTS();

  // printf("Stack overflow detected in task: %s\r\n", pcTaskName);


  for( ;; )
  {
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);
  }


}
/* USER CODE END 4 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */
  netTaskHandle = osThreadNew(StartNetTask, NULL, &netTask_attributes);
  /* Infinite loop */
  for(;;)
  {
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    osDelay(500);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

