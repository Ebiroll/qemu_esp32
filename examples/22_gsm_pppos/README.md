### PPP over Serial (PPPoS) client example

This example uses the [pppos patches](https://github.com/amuzyka-grinn/esp-idf) proposed by amuzyka-grinn.

Until those patches are not pulled to esp-idf, you must patch your **esp-idf**.

#### This is the required procedure:

1. copy all files from patches directory to the root of your esp-idf
2. go to esp-idf directory and execute *apply_ppp_patch.sh*
3. go back to this example directory
4. as with other ESP32 examples, set IDF-PATH variable and add 'xtensa-esp32-elf/bin' to your path
5. execute **make menuconfig**, enable **PPP support** (Component config → LWIP → Enable PPP support) and **configure your APN** (Component config → GSM configuration)
6. execute **make all** to build the example
7. execute **make flash** to flash the example

#### You can execute 'revert_ppp_patch.sh' in esp-idf to revert the changes before updating you esp-idf with 'git pull'


Before you run this example, make sure your GSM is powered on, in command mode, registered to network and connected to your ESP32 UART on pins #16 & #17 (hw flow controll is not used). 

The example runs as follows:

1. Creates the pppos client task which initializes modem on UART port and handles lwip interaction
2. When connection to the Internet is established, gets the current time using SNTP protocol
3. Creates http and https tasks synchronized with mutex
4. HTTP task gets text file from server and displays the header and data
5. HTTPS task gets ssl info from server and displays the header and received JSON data with info about used SSL
6. HTTP/HTTPS tasks repeats operation every 30 seconds


#### Tested with GSM SIM800L, should also work with other SIMCOM & Telit GSM modules.



```
I (8700) [PPPOS CLIENT]: AT
I (9200) [PPPOS CLIENT]: 
OK

I (9200) [PPPOS CLIENT]: ATE0
I (9700) [PPPOS CLIENT]: 
OK

I (9700) [PPPOS CLIENT]: AT+CPIN?
I (10200) [PPPOS CLIENT]: 
+CPIN: READY

OK

I (10200) [PPPOS CLIENT]: AT+CGDCONT=1,"IP","web.htgprs"
I (10700) [PPPOS CLIENT]: 
OK

I (10700) [PPPOS CLIENT]: ATDT*99***1#
I (11200) [PPPOS CLIENT]: 
CONNECT

I (11200) [PPPOS CLIENT]: Gsm init end
I (11200) [PPPOS CLIENT]: After pppapi_pppos_create
I (11200) [PPPOS CLIENT]: After pppapi_set_default
I (11200) [PPPOS CLIENT]: After pppapi_set_auth
I (11210) [PPPOS CLIENT]: After pppapi_connect, waiting
I (11870) [PPPOS CLIENT]: status_cb: Connected

I (11870) [PPPOS CLIENT]:    our_ipaddr  = 10.208.72.198

I (11870) [PPPOS CLIENT]:    his_ipaddr  = 10.64.64.64

I (11870) [PPPOS CLIENT]:    netmask     = 255.255.255.255

I (11880) [PPPOS CLIENT]:    our6_ipaddr = ::

I (12780) [SNTP]: OBTAINING TIME
I (12780) [SNTP]: Initializing SNTP
I (12780) [SNTP]: SNTP INITIALIZED
I (12780) [SNTP]: Waiting for system time to be set... (1/10)
I (14780) [SNTP]: TIME SET TO Mon Mar 20 15:54:08 2017

I (14780) [HTTP]: ===== HTTP GET REQUEST =========================================

I (15120) [HTTP]: DNS lookup succeeded. IP=82.196.4.208
I (15120) [HTTP]: ... allocated socket

I (15340) [HTTP]: ... connected
I (15340) [HTTP]: ... socket send success
I (15340) [HTTP]: ... reading HTTP response...
Header:
-------
HTTP/1.1 200 OK
Date: Mon, 20 Mar 2017 15:54:10 GMT
Server: Apache/2.4.7 (Ubuntu)
Last-Modified: Sat, 18 Mar 2017 17:32:44 GMT
ETag: "149-54b04ae918eb8"
Accept-Ranges: bytes
Content-Length: 329
Vary: Accept-Encoding
Content-Type: text/plain
-------
Data:
-----
Welcome to ESP32
The ESP32 is a low cost, low power microcontroller with integrated Wi-Fi & dual-mode Bluetooth,
which employs a dual-core Tensilica Xtensa LX6 microprocessor.
ESP32 is created and developed by Espressif Systems, a Shanghai-based Chinese company,
and is manufactured by TSMC using their 40 nm process.

2017 LoBo

-----
I (21280) [HTTP]: ... done reading from socket. 581 bytes read, 581 in buffer, errno=22

I (21290) [HTTP]: Waiting 30 sec...
I (21290) [HTTP]: ================================================================


I (21300) [HTTPS]: Seeding the random number generator
I (21310) [HTTPS]: Loading the CA root certificate...
I (21320) [HTTPS]: Setting hostname for TLS session...
I (21320) [HTTPS]: Setting up the SSL/TLS structure...
I (22330) [HTTPS]: ===== HTTPS GET REQUEST =========================================

I (22330) [HTTPS]: Connecting to www.howsmyssl.com:443...
I (22940) [HTTPS]: Connected.
I (22940) [HTTPS]: Performing the SSL/TLS handshake...
I (25400) [HTTPS]: Verifying peer X.509 certificate...
W (25400) [HTTPS]: Failed to verify peer certificate!
W (25400) [HTTPS]: verification info:   ! The certificate Common Name (CN) does not match with the expected CN

I (25410) [HTTPS]: Writing HTTP request...
I (25410) [HTTPS]: 102 bytes written
I (25420) [HTTPS]: Reading HTTP response...
I (26930) [HTTPS]: 5524 bytes read, 5524 in buffer
Header:
-------
HTTP/1.1 200 OK
Content-Length: 5289
Access-Control-Allow-Origin: *
Connection: close
Content-Type: application/json
Date: Mon, 20 Mar 2017 15:54:19 GMT
Strict-Transport-Security: max-age=631138519; includeSubdomains; preload
-------
I (26950) [HTTPS]: JSON data received.
I (26950) [HTTPS]: parsing JSON data:
given_cipher_suites = [Array] of 131 items
   TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
   TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
   TLS_DHE_RSA_WITH_AES_256_GCM_SHA384
   + 128 more...
ephemeral_keys_supported = True
session_ticket_supported = True
tls_compression_supported = False
unknown_cipher_suite_supported = False
beast_vuln = False
able_to_detect_n_minus_one_splitting = False
insecure_cipher_suites = {Object}
tls_version = TLS 1.2
rating = Probably Okay
I (27000) [HTTPS]: Waiting 30 sec...
I (27000) [HTTPS]: =================================================================
```
