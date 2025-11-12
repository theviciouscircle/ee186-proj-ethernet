# ee186-proj-ethernet

This repo contains the functionality for the ethernet part of the project. Documentation is below; ask me if you have questions.

## How It Works

The code enables a Nucleo board to communicate over ethernet to any computer.

The typical procedure is as such:

1. Attach the Arduino shield to the Nucleo board, and flash the code.
2. On the board's serial terminal, you should see something like the snippet below. Make sure the IP is `192.168.1.50` and the SN is `255.255.255.0`! If you get the exact same output, that means your board is ready to receive and transmit over ethernet.

```
1. Initializing W5500 chip...
2. Static IP configured:
MAC 02:AB:CD:01:00:01  IP 192.168.1.50  GW 192.168.1.1  SN 255.255.255.0  STATIC
TCP server listening on :5000
nc -v <ip> 5000
nc -v <ip> 5000   (type lines, press Enter)
```

3. Use an ethernet cable to connect the Arduino shield to your computer.
4. On your computer, set up a custom IP address on your Mac (more instructions below). 4. On your Mac, open Terminal, then enter `nc -v 192.168.1.50 5000`. You should see something like the snippet below.

```
% nc -v 192.168.1.50 5000
Connection to 192.168.1.50 port 5000 [tcp/commplex-main] succeeded!
```

5. With `nc -v` running, any messages sent from the Nucleo board to your Mac will be printed in the terminal automatically.
6. With `nc -v` running, you can send a message from your Mac to the Nucleo board by typing into your Mac's terminal then hitting enter/return.
7. To terminate the connection, control-C on your Mac's terminal.

More detailed instructions (for Mac) are below:

Set up a custom IP address on your Mac

1. Connect the Arduino shield to your Mac over ethernet.
2. On your Mac, open System Settings, and go to Network.
3. Select the Ethernet cable/adapter; this might appear under a name like `USB 10/100/1000 LAN`.
4. Click Details; this shows a pop-up menu.
5. On the left pane, click TCP/IP, then set the following:
   - Configure IPv4: Manually
   - IP address: 192.168.1.10
   - Subnet mask: 255.255.255.0
   - Router: [any, e.g. 192.168.1.1]

## Code

You have to make two main edits within `main.c`. First, make sure to include `net_w5500.h`! Look for the `/* USER CODE BEGIN Includes */`, and then paste this in:

```
/* USER CODE BEGIN Includes */

#include "net_w5500.h"

/* USER CODE END Includes */
```

Second, you need to edit `main()` to set up the ethernet connection, and send/receive messages. This is a quick idiom you can use within `main()`'s `/* USER CODE BEGIN 2*/`.

```
int main(void)
{
  /* USER CODE BEGIN 2 */

  // establishes the connection
  ethernet_start();
  net_start_server(NET_SERVER_PORT);

  for (;;) {

    // keep the socket serviced
    net_poll();

    // verify connection status
    int connected = net_is_connected();

    /**
     * this is how you send a message from the board to the computer.
     */
    if (connected) {
      net_send_str( /* YOUR STRING HERE */ );
    }

    /**
     * this is how you passively listen for messages from the computer to the board.
     * note: assumes that messages are shorter than 256 bytes.
     */
    char received_message[256];
    int n = net_recv_line(received_message, sizeof received_message, 0);
    if (n > 0) {
      /* THEN WE'VE RECEIVED A MESSAGE FROM THE COMPUTER */
      /* THE MESSAGE IS STORED INSIDE THE received_message VARIABLE */
      /* GO DO SOMETHING WITH IT */
    }
  }

  /* USER CODE END 2 */
}
```

## IDE Config

If you're editing the `.ioc` file, this is what you should do:

1. SPI1
   - Mode: Full-Duplex Master
   - Hardware NSS Signal: Disable
   - Frame Format: Motorola
   - Data Size: 8 Bits
   - First Bit: MSB First
   - Prescaler: 4
   - Baud Rate: [Default]
   - Clock Polarity: Low
   - Clock Phase: 1 Edge
   - CRC Calculation: Disabled
   - NSSP Mode: Disabled
   - NSS Signal Type: Software
2. LPUART1
   - Parameter Settings
   - Mode: Asynchronous
   - Hardware Flow Control (RS232): Disable
   - Hardware Flow Control (RS485): [Unchecked]
   - Baud Rate: 115200 Bits/s
   - Word Length: 8 Bits (including Parity)
   - Parity: None
   - Stop Bits: 1
   - Data Direction: Receive and Transmit
   - Single Sample: Disable
   - Prescaler: 1
   - Fifo Mode: Disable
   - Txfifo Threshold: 1 eighth full configuration
   - Rxfifo Threshold: 1 eighth full configuration
   - Auto Baudrate Mode: Disable
   - TX Pin Active Level Inversion: Disable
   - RX Pin Active Level Inversion: Disable
   - Data Inversion: Disable
3. GPIO > GPIO
   - PD14
     - Signal: n/a
     - GPIO Output Level: High
     - GPIO Mode: Output Push Pull
     - GPIO Pull-up/Pull-down: No pull-up and no pull-down
     - Maximum output speed: High
     - Fast Mode: n/a
   - PD14
     - Signal: n/a
     - GPIO Output Level: High
     - GPIO Mode: Output Push Pull
     - GPIO Pull-up/Pull-down: No pull-up and no pull-down
     - Maximum output speed: Low
     - Fast Mode: n/a
   - PD14
     - Signal: n/a
     - GPIO Output Level: Low
     - GPIO Mode: Output Push Pull
     - GPIO Pull-up/Pull-down: No pull-up and no pull-down
     - Maximum output speed: High
     - Fast Mode: n/a
   - PD14
     - Signal: n/a
     - GPIO Output Level: n/a
     - GPIO Mode: External Interrupt Mode with Falling edge trigger detection
     - GPIO Pull-up/Pull-down: No pull-up and no pull-down
     - Maximum output speed: n/a
     - Fast Mode: n/a
4. GPIO > SPI (you shouldn't need to change these, but check that they are right)
   - PA5
     - Signal on Pin: SPI1_SCK
     - GPIO Output Level: n/a
     - GPIO Mode: Alternate Function Push Pull
     - GPIO Pull-up/Pull-down: No pull-up and no pull-down
     - Maximum output speed: Very High
   - PA6
     - Signal on Pin: SPI1_MISO
     - GPIO Output Level: n/a
     - GPIO Mode: Alternate Function Push Pull
     - GPIO Pull-up/Pull-down: No pull-up and no pull-down
     - Maximum output speed: Very High
   - PA7
     - Signal on Pin: SPI1_MOSI
     - GPIO Output Level: n/a
     - GPIO Mode: Alternate Function Push Pull
     - GPIO Pull-up/Pull-down: No pull-up and no pull-down
     - Maximum output speed: Very High

After you generate code, you may need to do an additional config step. Go to `main.c`, Command-F for `/* USER CODE BEGIN MX_GPIO_Init_2 */`, and paste this in:

```
  /* USER CODE BEGIN MX_GPIO_Init_2 */
  // configure PA5/6/7 for SPI
  GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  /* USER CODE END MX_GPIO_Init_2 */
```

## Files

These files define helper functions for ethernet functionality:

- `Core/Inc/dhcp.h`
- `Core/Inc/dns.h`
- `Core/Inc/socket.h`
- `Core/Inc/w5500.h`
- `Core/Inc/wizchip_conf.h`
- `Core/Inc/net_w5500.h`
- `Core/Src/dhcp.c`
- `Core/Src/dns.c`
- `Core/Src/socket.c`
- `Core/Src/w5500.c`
- `Core/Src/wizchip_conf.c`
- `Core/Inc/net_w5500.c`

You should copy-paste these files into your project wholesale; do not edit them! All the `.c` files go in `Core/Src`, and all the `.h` files go in `Core/Inc`.

Note: when adding these files to `Core/Inc` and `Core/Src`, it's easiest to drag-and-drop them into the file browser pane of STM32CubeIDE. This makes the IDE recognise these files and compile them alongside the rest of the code.

Note: `net_w5500.h` and `net_w5500.c` were written by me; the rest of the files come wholesale from WizNet's ioLibrary.

## Wiring

You may need to do some wiring on the Arduino shield to connect three pins on the front to three pins on the rear. See the diagrams below; look for `[1]`, `[2]`, `[3]`.

Front of Arduino Shield:

```
                      |          |
 ---------------------|----------|---
|                     |          |   |
|                     | ETHERNET |   |
|                     |   PORT   |   |
|                     |          |   |
|                     |          |   |
|                      ----------    |
|                                  X |
|                                  X |
|                                  X |
| X                                X |
| X                               [1]|
| X                               [2]|
| X                               [3]|
| X                                X |
| X                                X |
| X                                X |
| X                                  |
|                                  X |
|                                  X |
| X                                X |
| X   ---------                    X |
| X  |         |                   X |
| X  | SD CARD |                   X |
| X  |  PORT   |                   X |
| X  |         |                   X |
|    |         |            _________|
 \                         /
  -------------------------
```

Back of Arduino Shield:

```
    |          |
 ---|----------|---------------------
|                                    |
|                                    |
|                                    |
|                                    |
|                                    |
|                                    |
| X                                  |
| X                                  |
| X                                  |
| X                                X |
| X                                X |
| X                                X |
| X                                X |
| X                                X |
| X                                X |
| X                                X |
|                                  X |
| X                                  |
| X                                  |
| X                                X |
| X                                X |
| X                                X |
| X                                X |
| X                                X |
| X           [2][1] X             X |
|_________     X [3] X               |
          \                         /
           -------------------------

```
