#pragma once
struct ArduinoLEDMatrix{uint8_t last[8][12]={};int begin(){return 1;}void renderBitmap(uint8_t (&frame)[8][12],int,int){memcpy(last,frame,sizeof(last));}};
