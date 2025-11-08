#include "net_w5500.h"

/* WIZnet ioLibrary headers you added under Middlewares/WIZnet/ioLibrary_Driver */
#include "wizchip_conf.h"
#include "w5500.h"
#include "socket.h"
#include "dhcp.h"
#include "dns.h"

#include <stdio.h>
#include <string.h>

#ifndef RET_OK
#define RET_OK 0
#endif
#ifndef RET_ERROR
#define RET_ERROR (-1)
#endif

/* Cube-generated handles */
extern SPI_HandleTypeDef  hspi1;
extern UART_HandleTypeDef hlpuart1;

/* ===== SPI ↔ W5500 glue (single and burst I/O, CS) ===== */

static inline void wizchip_select(void)
{
  HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin, GPIO_PIN_RESET);
}

static inline void wizchip_deselect(void)
{
  HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin, GPIO_PIN_SET);
}

static uint8_t wizchip_read(void)
{
  uint8_t tx = 0xFF, rx = 0x00;
  HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, HAL_MAX_DELAY);
  return rx;
}

static void wizchip_write(uint8_t wb)
{
  uint8_t dummy;
  HAL_SPI_TransmitReceive(&hspi1, &wb, &dummy, 1, HAL_MAX_DELAY);
}

static void wizchip_readburst(uint8_t *pBuf, uint16_t len)
{
  HAL_SPI_Receive(&hspi1, pBuf, len, HAL_MAX_DELAY);
}

static void wizchip_writeburst(uint8_t *pBuf, uint16_t len)
{
  HAL_SPI_Transmit(&hspi1, pBuf, len, HAL_MAX_DELAY);
}

static void wizchip_hw_reset(void)
{
  HAL_GPIO_WritePin(W5500_RST_GPIO_Port, W5500_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(2);   // >= 500 us
  HAL_GPIO_WritePin(W5500_RST_GPIO_Port, W5500_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(50);  // allow PLL/PHY settle
}

/* ===== Basic bring-up ===== */

int w5500_chip_init(void)
{
  uint8_t memsize[2][8] = {
    /*   S0 S1 S2 S3 S4 S5 S6 S7  */
    {2,  2,  2,  2,  2,  2,  2,  2},  /* TX: 2 KB per socket */
    {2,  2,  2,  2,  2,  2,  2,  2}   /* RX: 2 KB per socket */
  };

  wizchip_hw_reset();

  /* Register SPI/CS callbacks */
  reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
  reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);
  reg_wizchip_spiburst_cbfunc(wizchip_readburst, wizchip_writeburst);

  /* Initialize internal buffers */
  if (ctlwizchip(CW_INIT_WIZCHIP, (void*)memsize) != RET_OK) {
    printf("CW_INIT_WIZCHIP failed\r\n");
    return -1;
  }

  /* Optional: verify the chip version register (0x39) is 0x04. */
  if (getVERSIONR() != 0x04) {
    printf("Unexpected W5500 VERSIONR: 0x%02X\r\n", getVERSIONR());
    /* Not fatal, but worth noting */
  }

  return 0;
}

/* ===== Networking (DHCP + static fallback) ===== */

static uint8_t dhcp_workbuf[548];

/* You can change this MAC and the static fallback below. */
static wiz_NetInfo g_netinfo = {
  /* MAC (locally-administered: set bit1 of first byte) */
  .mac  = {0x02,0xAB,0xCD,0x01,0x00,0x01},
  .ip   = {0,0,0,0},
  .sn   = {255,255,255,0},
  .gw   = {0,0,0,0},
  .dns  = {8,8,8,8},
  .dhcp = NETINFO_DHCP
};

static const wiz_NetInfo g_static_fallback = {
  .mac  = {0x02,0xAB,0xCD,0x01,0x00,0x01},
  .ip   = {192,168,1,50},
  .sn   = {255,255,255,0},
  .gw   = {192,168,1,1},
  .dns  = {8,8,8,8},
  .dhcp = NETINFO_STATIC
};

void net_print_info(void)
{
  wiz_NetInfo cur;
  ctlnetwork(CN_GET_NETINFO, &cur);
  printf("MAC %02X:%02X:%02X:%02X:%02X:%02X  "
         "IP %d.%d.%d.%d  GW %d.%d.%d.%d  SN %d.%d.%d.%d  %s\r\n",
         cur.mac[0],cur.mac[1],cur.mac[2],cur.mac[3],cur.mac[4],cur.mac[5],
         cur.ip[0],cur.ip[1],cur.ip[2],cur.ip[3],
         cur.gw[0],cur.gw[1],cur.gw[2],cur.gw[3],
         cur.sn[0],cur.sn[1],cur.sn[2],cur.sn[3],
         (cur.dhcp==NETINFO_DHCP) ? "DHCP" : "STATIC");
}

/* DHCP_time_handler() must be called periodically (≈1 s) */
static inline void dhcp_tick_if_needed(uint32_t *last_ms)
{
  uint32_t now = HAL_GetTick();
  if (now - *last_ms >= 1000U) {
    DHCP_time_handler();
    *last_ms += 1000U;
  }
}

void ethernet_start(void)
{
  if (w5500_chip_init() != 0) {
    printf("wizchip init failed\r\n");
    return;
  }

  /* Set initial network info (at least MAC) */
  ctlnetwork(CN_SET_NETINFO, &g_netinfo);

  /* Try DHCP on socket 0 */
  printf("DHCP...\r\n");
  DHCP_init(0, dhcp_workbuf);

  uint32_t start = HAL_GetTick();
  uint32_t last_tick = start;
  int leased = 0;

  while ((HAL_GetTick() - start) < 15000U) {   /* 15 s window */
    dhcp_tick_if_needed(&last_tick);

    switch (DHCP_run()) {
      case DHCP_IP_LEASED:
        leased = 1;
        break;
      case DHCP_FAILED:
      case DHCP_STOPPED:
        leased = 0;
        break;
      default:
        break;
    }

    if (leased) break;
    HAL_Delay(50);
  }

  if (leased) {
    /* ioLibrary already applied the lease to chip; fetch+print */
    net_print_info();
  } else {
    printf("DHCP failed → using static fallback\r\n");
    ctlnetwork(CN_SET_NETINFO, (void*)&g_static_fallback);
    net_print_info();
  }
}

/* ===== Minimal TCP echo server on :5000 ===== */

void tcp_echo_task(void)
{
  const uint8_t  s     = 1;      /* socket index for echo */
  const uint16_t port  = 5000;
  uint8_t buf[1024];

  /* Ensure the socket is opened and listening */
  if (getSn_SR(s) == SOCK_CLOSED) {
    if (socket(s, Sn_MR_TCP, port, 0) != s) {
      /* couldn't open socket; try again later */
      HAL_Delay(5);
      return;
    }
    listen(s);
    return;  /* let main loop spin; we'll come back */
  }

  switch (getSn_SR(s)) {
    case SOCK_ESTABLISHED: {
      int32_t r = recv(s, buf, sizeof buf);
      if (r > 0) {
        /* Echo back exactly what we got. */
        int32_t sent = 0;
        while (sent < r) {
          int32_t n = send(s, buf + sent, (uint16_t)(r - sent));
          if (n < 0) break;
          sent += n;
        }
      } else if (r < 0) {
        /* Error → close and reopen next time */
        disconnect(s);
        close(s);
      }
    } break;

    case SOCK_CLOSE_WAIT:
      /* Peer closed; finish and close */
      disconnect(s);
      close(s);
      break;

    case SOCK_INIT:
      listen(s);
      break;

    case SOCK_LISTEN:
    case SOCK_SYNRECV:
    default:
      /* nothing to do */
      break;
  }
}

/* ===== Interrupt hook (optional but recommended) =====
 * Call this from HAL_GPIO_EXTI_Callback when (GPIO_Pin == W5500_INT_Pin).
 * We just clear all per-socket interrupt flags; your app can expand this
 * to react to specific events (CONNECT, DISCON, RECV, SENDOK, TIMEOUT). */
void net_w5500_irq_handler(void)
{
  for (uint8_t s = 0; s < 8; s++) {
    uint8_t sir = getSn_IR(s);
    if (sir) {
      /* Write-1-to-clear */
      setSn_IR(s, sir);
    }
  }
  /* Clear common IR too (if any has fired) */
  uint8_t cir = getIR();
  if (cir) setIR(cir);
}
