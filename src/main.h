#ifndef __MAIN_H__
#define __MAIN_H__

// local time zone definition
#define TIMEZONE "CST-8"

#define NTPSERVER1 "cn.ntp.org.cn"
#define NTPSERVER2 "ntp.ntsc.ac.cn"
#define NTPSERVER3 "ntp.aliyun.com"

#define HOSTNAME "arduino-esp8266"

#ifndef BIGIOT_DEVICE_ID
#warning "Please define BIGIOT_DEVICE_ID"
#define BIGIOT_DEVICE_ID "00000" // platform device id
#endif

#ifndef BIGIOT_API_KEY
#warning "Please define BIGIOT_API_KEY"
#define BIGIOT_API_KEY "00000000" // platform device api key
#endif

#ifndef BIGIOT_DATA_TEMP_ID
#warning "Please define BIGIOT_DATA_TEMP_ID"
#define BIGIOT_DATA_TEMP_ID "00000" // Temp stream data id
#endif

#ifndef BIGIOT_DATA_HUMI_ID
#warning "Please define BIGIOT_DATA_HUMI_ID"
#define BIGIOT_DATA_HUMI_ID "00000" // Humidity stream data id
#endif

#define WEB_CGI_PATH "/esp8266"
#define WEB_CONTENT_TEXT "text/plain"
#define WEB_CONTENT_HTML "text/html"
#define WEB_CONTENT_JSON "application/json"

#define WEB_RESULT_STATUS "{\"error\": {\"code\": 0}, \"result\": {\"status\": true}}"
#define WEB_HTML_INVALID "<h1>Invalid Request</h1>"
#define WEB_HTML_INDEX "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1, user-scalable=no'/><title>ESP8266</title><style>h1,a,button,label{width:100%;text-align:center}a,button,label{border:0;border-radius:0.3rem;line-height:2.4rem;font-size:1.2rem}button{background-color:#1fa3ec;color:#fff;cursor:pointer}a,label{display:inline-block;background-color:#d7d7d7}div{text-align:left;display:inline-block;min-width:260px}body{font-family:Arial,Helvetica,sans-serif;font-size:16px;color:#666;background-color:#f0f0f0;text-align:center}</style></head><body><div><h1>ESP8266</h1><hr/><h2><div>LED Status:</div><label id='led_status'>On</label></h2><hr/><h2><div>LED Ctrl:</div><button id='btn_led'onclick='setLedStatus()'>On/Off</button></h2><hr/><h2><div>Temperature:</div><label id='cur_temp'>0</label></h2><hr/><h2><div>Humidity:</div><label id='cur_humi'>0</label></h2><hr/><a href='/update'title='System Upgrade'target='_blank'rel='noopener noreferrer'>Upgrade</a></div></body><script>function callSync(method,params){try{var request={'method':method};if(params!=null){request.params=params}var requests=JSON.stringify(request);if(window.XMLHttpRequest){var xmlhttp=new XMLHttpRequest()}else if(window.ActiveXObject){var xmlhttp=new ActiveXObject('Microsoft.XMLHTTP')}xmlhttp.open('POST','/esp8266',false);xmlhttp.setRequestHeader('Content-Type','application/json');xmlhttp.send(requests);if((xmlhttp.status!=200)||(xmlhttp.responseText==null)||(xmlhttp.responseText=='')){return null}var result=JSON.parse(xmlhttp.responseText);console.log('callSync result: ',result);return result}catch(e){console.log('callSync error: '+e.name+': '+e.message);return null}}var Temperature=0;var Humidity=0;var LedStatus=true;var docCurTemp=document.getElementById('cur_temp');var docCurHumi=document.getElementById('cur_humi');var docLedStatus=document.getElementById('led_status');function getLedStatus(){LedStatus=false;var ret=callSync('led_status_show',null);if(ret!=null&&ret.result!=null){if(ret.result.status){LedStatus=true}else{LedStatus=false}}docLedStatus.innerHTML=LedStatus?'On':'Off'}function setLedStatus(){var ret=callSync('led_status_set',{'status':!LedStatus});if(ret!=null&&ret.error!=null){if(ret.error.code!=0){alert('Failed to set led status!');if(ret.error.message!=null){console.log(ret.error.message)}}}else{alert('Failed to call led status set api!')}getLedStatus()}function getTempHumi(){Temperature=0;Humidity=0;var ret=callSync('temp_humi_show',null);if(ret!=null&&ret.result!=null){if(ret.result.temp!=null){Temperature=ret.result.temp}if(ret.result.humi!=null){Humidity=ret.result.humi}console.log('Temperature: '+Temperature);console.log('Humidity: '+Humidity)}docCurTemp.innerHTML=Temperature;docCurHumi.innerHTML=Humidity}getLedStatus();getTempHumi();setInterval(function(){getLedStatus();getTempHumi()},5000);</script></html>"
#endif
