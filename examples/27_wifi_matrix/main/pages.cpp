void handleRoot() {
  server.send(200, "text/html", "<html><head><title>led-display</title></head><body><link rel=\"stylesheet\" href=\"/style.css\" />"
    "<script src=\"/ajax.js\"></script><script src=\"/default.js\"></script></body></html>");
}

void handleClear() {
  server.send(200, "application/json", "{\"error\": null}" );
  matrix.clearScreen();
}

void handleFill() {
  server.send(200, "application/json", "{\"error\": null}" );
  matrix.fillScreen();
}

// draw image directly.
void handleDraw() {

  String v = server.arg("v");

  if (v.length() != SCREEN_BYTES*2) {
    server.send(200, "application/json", "{\"error\":\"parameter error.\"}" );
    return;
  }

  uint8_t x = 0, y = 0;
  
  for (uint8_t i=0; i<SCREEN_BYTES; i++){ // 144 byte = 1152 bit = (24 * 3) * 16 pixel
     String bytestring = v.substring(i*2, (i*2)+2);
     const char *chars = bytestring.c_str();
     byte bytes = strtol(chars,NULL,16);
     for (uint8_t b = 0; b < 8; b++) {
       matrix.drawPixel(x, y, bitRead(bytes, 7-b));
       if (++x == LED_COLS) { y++; x=0; }
     }
  }

  matrix.writeScreen();

  server.send(200, "application/json", "{\"error\": null}" );
}

// save to memory
void handleSet() {

  String m = server.arg("m");
  String v = server.arg("v");

  if (m.length() == 0 || v.length() != SCREEN_BYTES*2) {
    server.send(200, "application/json", "{\"error\":\"parameter error.\"}" );
    return;
  }
  
  uint8_t k = SCREEN_MEMORY_START + (m.toInt() * SCREEN_BYTES);
  
  for (uint8_t i=0; i<SCREEN_BYTES; i++){ // 144 byte = 1152 bit = (24 * 3) * 16 pixel
     String bytestring = v.substring(i*2, (i*2)+2);
     const char *chars = bytestring.c_str();
     byte bytes = strtol(chars,NULL,16);
     EEPROM.write(k++, bytes);
  }

  EEPROM.commit();

  server.send(200, "application/json", "{\"error\": null}" );
}

// display image from memory.
void handleShow() {
  String m = server.arg("m");

  if (m.length() == 0) {
    server.send(200, "application/json", "{\"error\":\"parameter error.\"}" );
    return;
  }

  uint8_t k = SCREEN_MEMORY_START + (m.toInt() * SCREEN_BYTES);
  uint8_t x = 0, y = 0;
  for (uint8_t i=0; i<SCREEN_BYTES; i++){ // 144 byte = 1152 bit = (24 * 3) * 16 pixel
    byte bytes = (byte)EEPROM.read(k++);
     for (uint8_t b = 0; b < 8; b++) {
       matrix.drawPixel(x, y, bitRead(bytes, 7-b));
       if (++x == LED_COLS) { y++; x=0; }
     }
  }
  matrix.writeScreen();
  server.send(200, "application/json", "{\"error\": null}" );
}

void handleBright() {
  String v = server.arg("v");
  if (v.length() == 0) {
    server.send(200, "application/json", "{\"error\":\"parameter error.\"}" );
  }
  matrix.setBrightness(v.toInt());
  server.send(200, "application/json", "{\"error\": null}" );
}

void serve_ajax_js() {
  server.send(200, "text/javascript", "function ajax(url,cb){var http=new XMLHttpRequest();http.onreadystatechange=function(){\n"
    "if(http.readyState==4&&http.status==200){cb(http.responseText);}else{cb(null);}}\n"
    "http.open(\"GET\",url,true);http.send();}");
}

void serve_default_css() {
  server.send(200, "text/css", "table{border-collapse:collapse;}td{cursor:pointer;border:1px solid black;width:15px;height:18px;}.on{background:red;}.off{background:black;}");
}

void serve_default_js() {
  server.send(200, "text/javascript", "var data = new Array(1152);\ndata.fill(0);\nfunction draw() {"
    "ajax('/draw?v='+document.getElementById('hex').value, function(r){console.log(r);});}"
    "function calc_hex(){var tmp=(data+'').replace(/,/g,'');var result=[];while(tmp.length>0){"
    "var chr=parseInt(tmp.substring(0,4),2).toString(16);result.push(chr);tmp=tmp.substring(4);}"
    "document.getElementById('hex').value=result.join('');draw();}"
    "function toggle(el,pos){data[pos]=data[pos]==1?0:1;el.className=(data[pos]==1)?'on':'off';calc_hex();}"
    "document.write('<table>');for(var row=0;row<16;row++){document.write('<tr>');for(var col=0;col<72;col++){"
    "document.write(\"<td class='off' onclick='toggle(this,\"+(row*72+col)+\")'></td>\");}document.write('</tr>');}"
    "document.write('</table>');document.write(\"<textarea id='hex' cols='200' rows='3'></textarea>\");");
}

// 404 Not Found
void handleNotFound() {
  String message = "File Not Found\n\n";
  server.send ( 404, "text/plain", message );
}
