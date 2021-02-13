
//Webpage content
const char* webPageHeader = "\
<h1>Lora TX/RX </h1>\
<form action='/tx'>\
<label for='msg'>Message:</label><br>\
<input type='text' id='msg' name='msg' value=''><br>\
<input type='submit' value='tx'>\
</form>\
<br>\
<form action='/get'>\
<input type='submit' value='Update'>\
</form>\
<br>\
<form action='/rx'>\
<input type='submit' value='RX'>\
</form>\
<br>\
<form action='/cad'>\
<input type='submit' value='CAD'>\
</form>\
<br>\
<form action='/relay'>\
<input type='submit' value='Relay'>\
</form>\
<br>\
<form action='/set'>\
<label for='freq'>Frequency:</label><br>\
<input type='text' id='freq' name='freq' value=''><br>\
<label for='power'>Power:</label><br>\
<input type='text' id='power' name='power' value=''><br>\
<label for='sf'>SF:</label><br>\
<input type='text' id='sf' name='sf' value=''><br>\
<label for='syncword'>syncword:</label><br>\
<input type='text' id='syncword' name='syncword' value=''><br>\
<label for='crc'>crc:</label><br>\
<input type='text' id='crc' name='crc' value=''><br>\
<input type='submit' value='Set'>\
</form>";


void page_root (void) {
    webPage = webPageHeader;
    webPage += "<br>";
    webPage += PageConfig();
    server.send(200, "text/html", webPage);
    
  }
  
void page_tx (void) {
    char msg[255] = "";
    
   //   message += (server.method() == HTTP_GET) ? "GET" : "POST";
    for (uint8_t i = 0; i < server.args(); i++) {
      if (server.argName(i)=="msg") {
        //msg=server.arg(i);
        strcpy(msg, server.arg(i).c_str());
      }
    }
    if (msg != "") {
      lora->TX(msg, sizeof(msg));
      delay(2000);
    }
    webPage = webPageHeader;
    webPage += "<br>";
    webPage += PageConfig();
    webPage += "Sending...";
    server.send(200, "text/html", webPage);
}
void page_init (void) {   
    lora->Init(1,1);
    webPage = webPageHeader;
    webPage += "<br>";
    webPage += PageConfig();
    server.send(200, "text/html", webPage);
}

void page_get (void) { 
    webPage = webPageHeader;
    webPage += "<br>";
    webPage += PageConfig();
    server.send(200, "text/html", webPage);
}

void page_rx (void) {
    char msg [255] = "";   
    lora->RXContinuous(msg, sizeof(msg),10000);
    webPage = webPageHeader;
    webPage += "<br>";
    webPage += msg;
    server.send(200, "text/html", webPage);
}

void page_relay (void) {
    char msg [255] = "";   
    lora->RXContinuous(msg, sizeof(msg),0);
    webPage = webPageHeader;
    webPage += "<br>";
    webPage += msg;
    server.send(200, "text/html", webPage);
}

void page_cad (void) {
    char msg [255]= "";   
    lora->CAD(msg, sizeof(msg));
    webPage = webPageHeader;
    webPage += "<br>";
    webPage += msg;
    server.send(200, "text/html", webPage);
}
void page_set (void) {

    server.send(200, "text/html", webPage);
    for (uint8_t i = 0; i < server.args(); i++) {
      if (server.argName(i)=="freq" && server.arg(i)!="") {
        lora->Frequency(server.arg(i).toInt());
        Serial.printf ("freq Set to: %d\r\n", server.arg(i).toInt());
      }
      if (server.argName(i)=="power" && server.arg(i)!="") {
        lora->PowerDBm(server.arg(i).toInt());
        Serial.printf ("power Set to: %d\r\n", server.arg(i).toInt());
      }
      if (server.argName(i)=="sf" && server.arg(i)!="") {
        lora->SpreadingFactor(server.arg(i).toInt());
        Serial.printf ("sf Set to: %d\r\n", server.arg(i).toInt());
      }
      if (server.argName(i)=="crc" && server.arg(i)!="") {
        lora->RxPayloadCrcOn(server.arg(i).toInt());
        Serial.printf ("RxPayloadCrcOn Set to: %d\r\n", server.arg(i).toInt());
      }
      if (server.argName(i)=="syncword" && server.arg(i)!="") {
        lora->SyncWord(server.arg(i).toInt());
        Serial.printf ("SyncWord Set to: %d\r\n", server.arg(i).toInt());
      }
    }
    webPage = webPageHeader;
    webPage += "<br>";
    webPage += PageConfig();
}
  
void page_notfound (void) {
    webPage = "Not Found";
    server.send(200, "text/html", webPage);
}
String PageConfig (void) {
  String webPageConfig;
  webPageConfig += "<br>LongRangeMode = " + String(lora->LongRangeMode(),HEX);
  webPageConfig += "<br>AccessSharedReg = " + String(lora->AccessSharedReg(),HEX);
  webPageConfig += "<br>LowFrequencyModeOn = " + String(lora->LowFrequencyModeOn(),HEX);
  webPageConfig += "<br>Mode = " + String(lora->Mode(),HEX);
  webPageConfig += "<br>Frf = " + String(lora->Frf(),HEX);
  webPageConfig += "<br>PaSelect = " + String(lora->PaSelect(),HEX);
  webPageConfig += "<br>MaxPower = " + String(lora->MaxPower(),HEX);
  webPageConfig += "<br>OutputPower = " + String(lora->OutputPower(),HEX);
  webPageConfig += "<br>PaRamp = " + String(lora->PaRamp(),HEX);
  webPageConfig += "<br>OcpOn = " + String(lora->OcpOn(),HEX);
  webPageConfig += "<br>OcpTrim = " + String(lora->OcpTrim(),HEX);
  webPageConfig += "<br>LnaGain = " + String(lora->LnaGain(),HEX);
  webPageConfig += "<br>LnaBoostLf = " + String(lora->LnaBoostLf(),HEX);
  webPageConfig += "<br>LnaBoostHf = " + String(lora->LnaBoostHf(),HEX);
  webPageConfig += "<br>FifoAddrPtr = " + String(lora->FifoAddrPtr(),HEX);
  webPageConfig += "<br>FifoTxBaseAddr = " + String(lora->FifoTxBaseAddr(),HEX);
  webPageConfig += "<br>FifoRxBaseAddr = " + String(lora->FifoRxBaseAddr(),HEX);
  webPageConfig += "<br>FifoRxCurrentAddr = " + String(lora->FifoRxCurrentAddr(),HEX);
  webPageConfig += "<br>RxTimeoutMask = " + String(lora->RxTimeoutMask(),HEX);
  webPageConfig += "<br>RxDoneMask = " + String(lora->RxDoneMask(),HEX);
  webPageConfig += "<br>PayloadCrcErrorMask = " + String(lora->PayloadCrcErrorMask(),HEX);
  webPageConfig += "<br>ValidHeaderMask = " + String(lora->ValidHeaderMask(),HEX);
  webPageConfig += "<br>TxDoneMask = " + String(lora->TxDoneMask(),HEX);
  webPageConfig += "<br>CadDoneMask = " + String(lora->CadDoneMask(),HEX);
  webPageConfig += "<br>FhssChangeChannelMask = " + String(lora->FhssChangeChannelMask(),HEX);
  webPageConfig += "<br>CadDetectedMaskx = " + String(lora->CadDetectedMask(),HEX);
  webPageConfig += "<br>RxTimeout = " + String(lora->RxTimeout(),HEX);
  webPageConfig += "<br>RxDone = " + String(lora->RxDone(),HEX);
  webPageConfig += "<br>PayloadCrcError = " + String(lora->PayloadCrcError(),HEX);
  webPageConfig += "<br>ValidHeader = " + String(lora->ValidHeader(),HEX);
  webPageConfig += "<br>TxDone = " + String(lora->TxDone(),HEX);
  webPageConfig += "<br>CadDone = " + String(lora->CadDone(),HEX);
  webPageConfig += "<br>FhssChangeChannel = " + String(lora->FhssChangeChannel(),HEX);
  webPageConfig += "<br>CadDetected = " + String(lora->CadDetected(),HEX);
  webPageConfig += "<br>FifoRxBytesNb = " + String(lora->FifoRxBytesNb(),HEX);
  webPageConfig += "<br>ValidHeaderCnt = " + String(lora->ValidHeaderCnt(),HEX);
  webPageConfig += "<br>ValidPacketCnt = " + String(lora->ValidPacketCnt(),HEX);
  webPageConfig += "<br>RxCodingRate = " + String(lora->RxCodingRate(),HEX);
  webPageConfig += "<br>ModemStatus = " + String(lora->ModemStatus(),HEX);
  webPageConfig += "<br>PacketSnr = " + String(lora->PacketSnr(),HEX);
  webPageConfig += "<br>PacketRssi = " + String(lora->PacketRssi(),HEX);
  webPageConfig += "<br>Rssi = " + String(lora->Rssi(),HEX);
  webPageConfig += "<br>PllTimeout = " + String(lora->PllTimeout(),HEX);
  webPageConfig += "<br>CrcOnPayload = " + String(lora->CrcOnPayload(),HEX);
  webPageConfig += "<br>FhssPresentChannel = " + String(lora->FhssPresentChannel(),HEX);
  webPageConfig += "<br>Bw = " + String(lora->Bw(),HEX);
  webPageConfig += "<br>CodingRate = " + String(lora->CodingRate(),HEX);
  webPageConfig += "<br>ImplicitHeaderModeOn = " + String(lora->ImplicitHeaderModeOn(),HEX);
  webPageConfig += "<br>SpreadingFactor = " + String(lora->SpreadingFactor(),HEX);
  webPageConfig += "<br>TxContinuousMode = " + String(lora->TxContinuousMode(),HEX);
  webPageConfig += "<br>RxPayloadCrcOn = " + String(lora->RxPayloadCrcOn(),HEX);
  webPageConfig += "<br>SymbTimeout = " + String(lora->SymbTimeout(),HEX);
  webPageConfig += "<br>PreambleLength = " + String(lora->PreambleLength(),HEX);
  webPageConfig += "<br>PayloadLength = " + String(lora->PayloadLength(),HEX);
  webPageConfig += "<br>PayloadMaxLength = " + String(lora->PayloadMaxLength(),HEX);
  webPageConfig += "<br>FreqHoppingPeriod = " + String(lora->FreqHoppingPeriod(),HEX);
  webPageConfig += "<br>FifoRxByteAddrPtr = " + String(lora->FifoRxByteAddrPtr(),HEX);
  webPageConfig += "<br>LowDataRateOptimize = " + String(lora->LowDataRateOptimize(),HEX);
  webPageConfig += "<br>AgcAutoOn = " + String(lora->AgcAutoOn(),HEX);
  webPageConfig += "<br>PpmCorrection = " + String(lora->PpmCorrection(),HEX);
  webPageConfig += "<br>FreqError = " + String(lora->FreqError(),HEX);
  webPageConfig += "<br>RssiWideband = " + String(lora->RssiWideband(),HEX);
  webPageConfig += "<br>IfFreq2 = " + String(lora->IfFreq2(),HEX);
  webPageConfig += "<br>IfFreq1 = " + String(lora->IfFreq1(),HEX);
  webPageConfig += "<br>AutomaticIFOn = " + String(lora->AutomaticIFOn(),HEX);
  webPageConfig += "<br>DetectionOptimize = " + String(lora->DetectionOptimize(),HEX);
  webPageConfig += "<br>InvertIQ_RX = " + String(lora->InvertIQ_RX(),HEX);
  webPageConfig += "<br>InvertIQ_TX = " + String(lora->InvertIQ_TX(),HEX);
  webPageConfig += "<br>HighBWOptimize1 = " + String(lora->HighBWOptimize1(),HEX);
  webPageConfig += "<br>DetectionThreshold = " + String(lora->DetectionThreshold(),HEX);
  webPageConfig += "<br>SyncWord = " + String(lora->SyncWord(),HEX);
  webPageConfig += "<br>HighBWOptimize2 = " + String(lora->HighBWOptimize2(),HEX);
  webPageConfig += "<br>InvertIQ2 = " + String(lora->InvertIQ2(),HEX);
  webPageConfig += "<br>Dio0Mapping= " + String(lora->Dio0Mapping(),HEX);
  webPageConfig += "<br>Dio1Mapping= " + String(lora->Dio1Mapping(),HEX);
  webPageConfig += "<br>Dio2Mapping= " + String(lora->Dio2Mapping(),HEX);
  webPageConfig += "<br>Dio3Mapping= " + String(lora->Dio3Mapping(),HEX);
  webPageConfig += "<br>Dio4Mapping= " + String(lora->Dio4Mapping(),HEX);
  webPageConfig += "<br>Dio5Mapping= " + String(lora->Dio5Mapping(),HEX);
  webPageConfig += "<br>Version= " + String(lora->Version(),HEX);
  webPageConfig += "<br>PaDac= " + String(lora->PaDac(),HEX);
  webPageConfig += "<br>FormerTemp= " + String(lora->FormerTemp(),HEX);
  webPageConfig += "<br>AgcReferenceLevel= " + String(lora->AgcReferenceLevel(),HEX);
  webPageConfig += "<br>AgcStep1= " + String(lora->AgcStep1(),HEX);
  webPageConfig += "<br>AgcStep2= " + String(lora->AgcStep2(),HEX);
  webPageConfig += "<br>AgcStep3= " + String(lora->AgcStep3(),HEX);
  webPageConfig += "<br>AgcStep4= " + String(lora->AgcStep4(),HEX);
  webPageConfig += "<br>AgcStep5= " + String(lora->AgcStep5(),HEX);
  webPageConfig += "<br>PllBandwidth= " + String(lora->PllBandwidth(),HEX);
  return webPageConfig;
}
