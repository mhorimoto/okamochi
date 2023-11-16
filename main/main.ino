//================================================================
//  UECS対応制御装置
//  By 九州大学大学院農学研究院環境農学部門農業生産システム設計学研究室
//  Modifier: Masafumi Horimoto
//  Original:
//     岡安崇史
//     E-mail:okayasu@bpes.kyushu-u.ac.jp
//
//  Based on UARDECS Sample Program
//  Ver1.2
//  By H.kurosaki 2015/12/10
//================================================================

#include <avr/pgmspace.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <Ethernet2.h>
#include <EEPROM.h>
#include <AT24CX.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <Uardecs_mega.h>

/*** Define for LCD ***/
#define RS     37
#define RW     38
#define ENA    39
#define DB0    40
#define DB1    41
#define DB2    42
#define DB3    43
#define DB4    44
#define DB5    45
#define DB6    46
#define DB7    47

/*** Define for Arrow Key ***/
#define SW_SAFE    3
#define SW_RLY    31
#define SW_ENTER  32
#define SW_UP     33
#define SW_DOWN   34
#define SW_LEFT   35
#define SW_RIGHT  36
#define SELECT_VR A15

/*** Define Relay Ports ***/
#define  RLY1    22
#define  RLY2    23
#define  RLY3    24
#define  RLY4    25
#define  RLY5    26
#define  RLY6    27
#define  RLY7    28
#define  RLY8    29

/*** Ethernet2 Ports ***/
#define NICSS    53

/*** I2C Address ***/
#define AT24LC254_ADDR   0x50
#define AT24LC254_INDEX  0

/*** EEPROM LOWCORE ASSIGN ***/
#define LC_UECS_ID  0
#define LC_MAC      6
#define LC_START    0x20
#define LC_END      0x7fff

LiquidCrystal lcd(RS,RW,ENA,DB0,DB1,DB2,DB3,DB4,DB5,DB6,DB7);
AT24C256      atmem(0);
EthernetClient client;

char pgname[21],lcdtxt[21],lcdscr[4][21];
int  lcdmenu=0;
/////////////////////////////////////
//IP reset jupmer pin setting
//IPアドレスリセット用ジャンパーピン設定
/////////////////////////////////////
//Pin ID. This pin is pull-upped automatically.
//ピンIDを入力、このピンは自動的にプルアップされます
//ピンIDは変更可能です
const byte U_InitPin = SW_SAFE;
const byte U_InitPin_Sense=LOW;

//When U_InitPin status equals this value,IP address is set "192.168.1.7".
//(This is added Ver0.3 over)
//U_InitPinに指定したピンがこの値になった時、IPアドレスが"192.168.1.7"に初期化されます。
//購入直後のArduinoは初期化が必要です。

////////////////////////////////////
//Node basic infomation
//ノードの基本情報
///////////////////////////////////
const char U_name[] PROGMEM= "M304jp Node v1.20\0";//MAX 20 chars
//prog_uchar PU_name[] PROGMEM ="M304jp Node v1.20\0";
const char U_vender[] PROGMEM= "HOLLY&Co.Ltd.";//MAX 20 chars
const char U_uecsid[] PROGMEM= "10100C00000B";//12 chars fixed
const char U_footnote[] PROGMEM= "M304jp UARDECS version";
//const int U_footnoteLetterNumber = 48;//Abolished after Ver 0.6
char U_nodename[20] = "M304jp";//MAX 19chars (This value enabled in safemode)
UECSOriginalAttribute U_orgAttribute;//この定義は弄らないで下さい
//////////////////////////////////
// html page1 setting
//Web上の設定画面に関する宣言
//////////////////////////////////
#define DECIMAL_DIGIT  1 //小数桁数
#define DECIMAL_DIGIT_0  0 //小数桁数

//Total number of HTML table rows.
//web設定画面で表示すべき項目の総数
const int U_HtmlLine = 16;

//●表示素材の定義(1)数値表示
//UECSSHOWDATA
const char NAME0[] PROGMEM= "Temperature";
const char UNIT0[] PROGMEM= "C";
const char NOTE0[] PROGMEM= "SHOWDATA";

//表示用の値を格納する変数
//小数桁数が1の場合、123が12.3と表示される
signed long showValueTemp;

//●表示素材の定義(2)選択肢表示
//UECSSELECTDATA
const char NAME1[] PROGMEM= "UserSwitch";
const char NOTE1[] PROGMEM= "SELECTDATA";
const char UECSOFF[] PROGMEM= "OFF";
const char UECSON[] PROGMEM= "ON";
const char UECSAUTO[] PROGMEM= "AUTO";
const char *stringSELECT[3]={
  UECSOFF,
  UECSON,
  UECSAUTO,
};

//入力された選択肢の位置を受け取る変数
//UECSOFFが0、UECSONで1、UECSAUTOで2になる
signed long setONOFFAUTO_Temp;

//●表示素材の定義(3)数値入力
//UECSINPUTDATA
const char NAME2[] PROGMEM= "SetTemp";
const char UNIT2[] PROGMEM= "C";
const char NOTE2[] PROGMEM= "INPUTDATA";

//入力された数値を受け取る変数
//小数桁数が1の場合、例えばWeb上で12.3が入力されると123が代入される
signed long setONTempFromWeb;

//●表示素材の定義(4)文字表示
//UECSSHOWSTRING
const char NAME3[] PROGMEM= "Now status";
const char NOTE3[] PROGMEM= "SHOWSTRING";
const char SHOWSTRING_OFF[] PROGMEM= "OUTPUT:OFF";
const char SHOWSTRING_ON [] PROGMEM= "OUTPUT:ON";
const char *stringSHOW[2]={
  SHOWSTRING_OFF,
  SHOWSTRING_ON,
};
signed long showValueStatusTemp;

//●表示素材の定義(1)数値表示
//UECSSHOWDATA
const char NAME4[] PROGMEM= "Humidity";
const char UNIT4[] PROGMEM= "%";
const char NOTE4[] PROGMEM= "SHOWDATA";

//表示用の値を格納する変数
//小数桁数が1の場合、123が12.3と表示される
signed long showValueHumidity;

//●表示素材の定義(2)選択肢表示
//UECSSELECTDATA
const char NAME5[] PROGMEM= "UserSwitch";
const char NOTE5[] PROGMEM= "SELECTDATA";
const char *stringSELECT_Humidity[3]={
  UECSOFF,
  UECSON,
  UECSAUTO,
};

//入力された選択肢の位置を受け取る変数
//UECSOFFが0、UECSONで1、UECSAUTOで2になる
signed long setONOFFAUTO_Humidity;

//●表示素材の定義(3)数値入力
//UECSINPUTDATA
const char NAME6[] PROGMEM= "SetHumidity";
const char UNIT6[] PROGMEM= "%";
const char NOTE6[] PROGMEM= "INPUTDATA";

//入力された数値を受け取る変数
//小数桁数が1の場合、例えばWeb上で12.3が入力されると123が代入される
signed long setONHumidityFromWeb;

//●表示素材の定義(4)文字表示
//UECSSHOWSTRING
const char NAME7[] PROGMEM= "Now status";
const char NOTE7[] PROGMEM= "SHOWSTRING";
const char *stringSHOW_Humidity[2]={
  SHOWSTRING_OFF,
  SHOWSTRING_ON,
};
signed long showValueStatusHumidity;

//●表示素材の定義(1)数値表示
//UECSSHOWDATA
const char NAME8[] PROGMEM= "Illuminance";
const char UNIT8[] PROGMEM= "";
const char NOTE8[] PROGMEM= "SHOWDATA";

//表示用の値を格納する変数
//小数桁数が1の場合、123が12.3と表示される
signed long showValueIlluminance;

//●表示素材の定義(2)選択肢表示
//UECSSELECTDATA
const char NAME9[] PROGMEM= "UserSwitch";
const char NOTE9[] PROGMEM= "SELECTDATA";
const char *stringSELECT_Illuminance[3]={
  UECSOFF,
  UECSON,
  UECSAUTO,
};

//入力された選択肢の位置を受け取る変数
//UECSOFFが0、UECSONで1、UECSAUTOで2になる
signed long setONOFFAUTO_Illuminance;

//●表示素材の定義(3)数値入力
//UECSINPUTDATA
const char NAME10[] PROGMEM= "SetIlluminance";
const char UNIT10[] PROGMEM= "";
const char NOTE10[] PROGMEM= "INPUTDATA";

//入力された数値を受け取る変数
//小数桁数が1の場合、例えばWeb上で12.3が入力されると123が代入される
signed long setONIlluminanceFromWeb;

//●表示素材の定義(4)文字表示
//UECSSHOWSTRING
const char NAME11[] PROGMEM= "Now status";
const char NOTE11[] PROGMEM= "SHOWSTRING";
const char *stringSHOW_Illuminance[2]={
  SHOWSTRING_OFF,
  SHOWSTRING_ON,
};
signed long showValueStatusIlluminance;


//●表示素材の定義(1)数値表示
//UECSSHOWDATA
const char NAME12[] PROGMEM= "REncoder";
const char UNIT12[] PROGMEM= "";
const char NOTE12[] PROGMEM= "SHOWDATA";

//表示用の値を格納する変数
//小数桁数が1の場合、123が12.3と表示される
signed long showValueREncoder;

//●表示素材の定義(2)選択肢表示
//UECSSELECTDATA
const char NAME13[] PROGMEM= "UserSwitch";
const char NOTE13[] PROGMEM= "SELECTDATA";
const char *stringSELECT_REncoder[3]={
  UECSOFF,
  UECSON,
  UECSAUTO,
};

//入力された選択肢の位置を受け取る変数
//UECSOFFが0、UECSONで1、UECSAUTOで2になる
signed long setONOFFAUTO_REncoder;

//●表示素材の定義(3)数値入力
//UECSINPUTDATA
const char NAME14[] PROGMEM= "SetREncoder";
const char UNIT14[] PROGMEM= "";
const char NOTE14[] PROGMEM= "INPUTDATA";

//入力された数値を受け取る変数
//小数桁数が1の場合、例えばWeb上で12.3が入力されると123が代入される
signed long setONREncoderFromWeb;

//●表示素材の定義(4)文字表示
//UECSSHOWSTRING
const char NAME15[] PROGMEM= "Now status";
const char NOTE15[] PROGMEM= "SHOWSTRING";
const char *stringSHOW_REncoder[2]={
  SHOWSTRING_OFF,
  SHOWSTRING_ON,
};
signed long showValueStatusREncoder;

//●ダミー素材の定義
//dummy value
const char NONES[] PROGMEM= "";
const char** DUMMY = NULL;

//表示素材の登録
struct UECSUserHtml U_html[U_HtmlLine]={
//{名前,入出力形式  ,単位 ,詳細説明,選択肢文字列  ,選択肢数,値     ,最小値,最大値,小数桁数}
  {NAME0, UECSSHOWDATA  ,UNIT0  ,NOTE0  , DUMMY   , 0 , &(showValueTemp)  , 0, 0, DECIMAL_DIGIT},
  {NAME1, UECSSELECTDATA  ,NONES  ,NOTE1  , stringSELECT  , 3 , &(setONOFFAUTO_Temp)  , 0, 0, DECIMAL_DIGIT_0},
  {NAME2, UECSINPUTDATA ,UNIT2  ,NOTE2  , DUMMY   , 0 , &(setONTempFromWeb) , 100, 1000, DECIMAL_DIGIT},
  {NAME3, UECSSHOWSTRING  ,NONES  ,NOTE3  , stringSHOW  , 2 , &(showValueStatusTemp)  , 0, 0, DECIMAL_DIGIT_0},
  {NAME4, UECSSHOWDATA  ,UNIT4  ,NOTE4  , DUMMY   , 0 , &(showValueHumidity)  , 0, 0, DECIMAL_DIGIT_0},
  {NAME5, UECSSELECTDATA  ,NONES  ,NOTE5  , stringSELECT_Humidity , 3 , &(setONOFFAUTO_Humidity)  , 0, 0, DECIMAL_DIGIT_0},
  {NAME6, UECSINPUTDATA ,UNIT6  ,NOTE6  , DUMMY   , 0 , &(setONHumidityFromWeb) , 10, 100, DECIMAL_DIGIT_0},
  {NAME7, UECSSHOWSTRING  ,NONES  ,NOTE7  , stringSHOW_Humidity , 2 , &(showValueStatusHumidity)  , 0, 0, DECIMAL_DIGIT_0},
  {NAME8, UECSSHOWDATA  ,UNIT8  ,NOTE8  , DUMMY   , 0 , &(showValueIlluminance)  , 0, 0, DECIMAL_DIGIT_0},
  {NAME9, UECSSELECTDATA  ,NONES  ,NOTE9  , stringSELECT_Illuminance , 3 , &(setONOFFAUTO_Illuminance)  , 0, 0, DECIMAL_DIGIT_0},
  {NAME10, UECSINPUTDATA ,UNIT10  ,NOTE10, DUMMY   , 0 , &(setONIlluminanceFromWeb) , 0, 1023, DECIMAL_DIGIT_0},
  {NAME11, UECSSHOWSTRING  ,NONES  ,NOTE11  , stringSHOW_Illuminance , 2 , &(showValueStatusIlluminance)  , 0, 0, DECIMAL_DIGIT_0},
  {NAME12, UECSSHOWDATA  ,UNIT12  ,NOTE12  , DUMMY   , 0 , &(showValueREncoder)  , 0, 0, DECIMAL_DIGIT_0},
  {NAME13, UECSSELECTDATA  ,NONES  ,NOTE13  , stringSELECT_REncoder , 3 , &(setONOFFAUTO_REncoder)  , 0, 0, DECIMAL_DIGIT_0},
  {NAME14, UECSINPUTDATA ,UNIT14  ,NOTE14  , DUMMY   , 0 , &(setONREncoderFromWeb) , 0, 1023, DECIMAL_DIGIT_0},
  {NAME15, UECSSHOWSTRING  ,NONES  ,NOTE15  , stringSHOW_REncoder , 2 , &(showValueStatusREncoder)  , 0, 0, DECIMAL_DIGIT_0},
};

//////////////////////////////////
// UserCCM setting
// CCM用の素材
//////////////////////////////////

//define CCMID for identify
//CCMID_dummy must put on last
//可読性を高めるためCCMIDという記号定数を定義しています
enum {
  CCMID_InAirTemp,
  CCMID_CndInAirTemp,
  CCMID_InAirHumid,
  CCMID_CndInAirHumid,
  CCMID_Illuminance,
  CCMID_CndIlluminance,
  CCMID_REncoder,
  CCMID_CndREncoder,
  CCMID_dummy, //CCMID_dummyは必ず最後に置くこと
};

//CCM格納変数の宣言
//ここはこのままにして下さい
const int U_MAX_CCM = CCMID_dummy;
UECSCCM U_ccmList[U_MAX_CCM];

//CCM定義用の素材
const char ccmNameTemp[] PROGMEM= "Temperature";
const char ccmTypeTemp[] PROGMEM= "InAirTemp.mIC";
const char ccmUnitTemp[] PROGMEM= "C";

const char ccmNameCndTemp[] PROGMEM= "NodeCond_Temp";
const char ccmTypeCndTemp[] PROGMEM= "CndTemp.aXX";
const char ccmUnitCndTemp[] PROGMEM= "";

const char ccmNameHumid[] PROGMEM= "AirHumid";
const char ccmTypeHumid[] PROGMEM= "InAirHumid.mIC";
const char ccmUnitHumid[] PROGMEM= "C";

const char ccmNameCndHumid[] PROGMEM= "NodeCond_Humid";
const char ccmTypeCndHumid[] PROGMEM= "CndHumid.aXX";
const char ccmUnitCndHumid[] PROGMEM= "";

const char ccmNameIlluminance[] PROGMEM= "Illuminance";
const char ccmTypeIlluminance[] PROGMEM= "Illuminance.mIC";
const char ccmUnitIlluminance[] PROGMEM= "";

const char ccmNameCndIlluminance[] PROGMEM= "NodeCond_Illuminance";
const char ccmTypeCndIlluminance[] PROGMEM= "CndIlluminance.aXX";
const char ccmUnitCndIlluminance[] PROGMEM= "";

const char ccmNameREncoder[] PROGMEM= "REncoder";
const char ccmTypeREncoder[] PROGMEM= "REncoder.mIC";
const char ccmUnitREncoder[] PROGMEM= "";

const char ccmNameCndREncoder[] PROGMEM= "NodeCond_REncoder";
const char ccmTypeCndREncoder[] PROGMEM= "CndREncoder.aXX";
const char ccmUnitCndREncoder[] PROGMEM= "";

//------------------------------------------------------
//UARDECS初期化用関数
//主にCCMの作成とMACアドレスの設定を行う
//------------------------------------------------------
void UserInit() {
//MAC address is printed on sticker of Ethernet Shield.
//You must assign unique MAC address to each nodes.
//MACアドレス設定、必ずEthernet Shieldに書かれた値を入力して下さい。
//全てのノードに異なるMACアドレスを設定する必要があります。
  U_orgAttribute.mac[0] = 0x02;
  U_orgAttribute.mac[1] = 0xA2;
  U_orgAttribute.mac[2] = 0x73;
  U_orgAttribute.mac[3] = 0x0B;
  U_orgAttribute.mac[4] = 0x00;
  U_orgAttribute.mac[5] = 0x02;
  
//Set ccm list
//CCMの作成
//UECSsetCCM(送/受の別, CCMID(固有の整数), CCM名(表示用), type, 単位,優先度(通常29), 小数桁数, 送受信頻度);
//true:送信 false:受信になります
  UECSsetCCM(false, CCMID_InAirTemp, ccmNameTemp, ccmTypeTemp, ccmUnitTemp, 29, DECIMAL_DIGIT, A_10S_0);
  UECSsetCCM(true,  CCMID_CndInAirTemp, ccmNameCndTemp, ccmTypeCndTemp, ccmUnitCndTemp, 29, DECIMAL_DIGIT_0, A_1S_0);
  UECSsetCCM(false, CCMID_InAirHumid, ccmNameHumid, ccmTypeHumid, ccmUnitHumid, 29, DECIMAL_DIGIT_0, A_10S_0);
  UECSsetCCM(true,  CCMID_CndInAirHumid, ccmNameCndHumid, ccmTypeCndHumid, ccmUnitCndHumid, 29,  DECIMAL_DIGIT_0, A_1S_0);
  UECSsetCCM(false, CCMID_Illuminance, ccmNameIlluminance, ccmTypeIlluminance, ccmUnitIlluminance, 29, DECIMAL_DIGIT_0, A_10S_0);
  UECSsetCCM(true,  CCMID_CndIlluminance, ccmNameCndIlluminance, ccmTypeCndIlluminance, ccmUnitCndIlluminance, 29,  DECIMAL_DIGIT_0, A_1S_0);
  UECSsetCCM(false, CCMID_REncoder, ccmNameREncoder, ccmTypeREncoder, ccmUnitREncoder, 29, DECIMAL_DIGIT_0, A_10S_0);
  UECSsetCCM(true,  CCMID_CndREncoder, ccmNameCndREncoder, ccmTypeCndREncoder, ccmUnitCndREncoder, 29,  DECIMAL_DIGIT_0, A_1S_0);
}

//---------------------------------------------------------
//Webページから入力が行われ各種値を取得後以下の関数が呼び出される。
//この関数呼び出し後にEEPROMへの値の保存とWebページの再描画が行われる
//---------------------------------------------------------
void OnWebFormRecieved(){
  ChangeThermostat();
}


//---------------------------------------------------------
//毎秒１回呼び出される関数
//関数の終了後に自動的にCCMが送信される
//---------------------------------------------------------
void UserEverySecond(){
  ChangeThermostat();
}

//---------------------------------------------------------
//１分に１回呼び出される関数
//---------------------------------------------------------
void UserEveryMinute(){
}

//---------------------------------------------------------
//メインループ
//システムのタイマカウント，httpサーバーの処理，
//UDP16520番ポートと16529番ポートの通信文をチェックした後，呼び出さされる関数。
//呼び出される頻度が高いため，重い処理を記述しないこと。
//---------------------------------------------------------
void UserEveryLoop(){
  
}

//---------------------------------------------------------
//setup()実行後に呼び出されるメインループ
//この関数内ではUECSloop()関数を呼び出さなくてはならない。
//UserEveryLoop()に似ているがネットワーク関係の処理を行う前に呼び出される。
//必要に応じて処理を記述してもかまわない。
//呼び出される頻度が高いため,重い処理を記述しないこと。
//---------------------------------------------------------
void loop(){
  UECSloop();
  dispScr(lcdmenu);
  //  lcd.setCursor(18,0);
  //  lcd.print(lcdmenu);
}

//---------------------------------------------------------
//起動直後に１回呼び出される関数。
//様々な初期化処理を記述できる。
//この関数内ではUECSsetup()関数を呼び出さなくてはならない。
//必要に応じて処理を記述してもかまわない。
//---------------------------------------------------------
void setup(){
  Serial.begin(115200);
  Ethernet.init(53);
  pinMode(SW_ENTER,INPUT_PULLUP);
  pinMode(SW_UP,INPUT_PULLUP);
  pinMode(SW_DOWN,INPUT_PULLUP);
  pinMode(SW_LEFT,INPUT_PULLUP);
  pinMode(SW_RIGHT,INPUT_PULLUP);
  pinMode(SW_SAFE,INPUT_PULLUP);
  pinMode(SW_RLY,INPUT);
  pinMode(SELECT_VR,INPUT);
  pinMode(RLY1,OUTPUT);
  digitalWrite(RLY1,HIGH);
  pinMode(RLY2,OUTPUT);
  digitalWrite(RLY2,HIGH);
  pinMode(RLY3,OUTPUT);
  digitalWrite(RLY3,HIGH);
  pinMode(RLY4,OUTPUT);
  digitalWrite(RLY4,HIGH);
  pinMode(RLY5,OUTPUT);
  digitalWrite(RLY5,HIGH);
  pinMode(RLY6,OUTPUT);
  digitalWrite(RLY6,HIGH);
  pinMode(RLY7,OUTPUT);
  digitalWrite(RLY7,HIGH);
  pinMode(RLY8,OUTPUT);
  digitalWrite(RLY8,HIGH);
  lcd.begin(20,4);
  lcd.setCursor(0,0);
  dispScr(0);
  ///  lcd.print(lcdtxt);
  UECSsetup();
}

//---------------------------------------------------------
//サーモスタット動作を変化させる関数
//---------------------------------------------------------
void ChangeThermostat(){
//温度 やめる
  showValueTemp = U_ccmList[CCMID_InAirTemp].value;
  if(setONOFFAUTO_Temp==0) {
    U_ccmList[CCMID_CndInAirTemp].value=0;
    Serial.print("TEMP-OFF\r\n");
    sidewindow(0); // STOP
    //    digitalWrite(LED_Blue_Pin, LOW);
    //    digitalWrite(D0_Pin, LOW);
  } else if(setONOFFAUTO_Temp==1) {
    // Manual ON
    U_ccmList[CCMID_CndInAirTemp].value=1;
    Serial.println("TEMP-ON");
    sidewindow(0);
    //    digitalWrite(LED_Blue_Pin, HIGH);
    //    digitalWrite(D0_Pin, HIGH);
  }
  else if(setONOFFAUTO_Temp==2 && U_ccmList[CCMID_InAirTemp].validity && U_ccmList[CCMID_InAirTemp].value<setONTempFromWeb) {
    U_ccmList[CCMID_CndInAirTemp].value=1;
    Serial.println("TEMP-ON");
    //    digitalWrite(LED_Blue_Pin, HIGH);
    //    digitalWrite(D0_Pin, HIGH);
  }//Auto ON
  else {
    U_ccmList[CCMID_CndInAirTemp].value=0;
    Serial.println("TEMP-OFF");
    //    digitalWrite(LED_Blue_Pin, LOW);
    //     digitalWrite(D0_Pin, LOW);
 }//OFF
  showValueStatusTemp = U_ccmList[CCMID_CndInAirTemp].value;
  Serial.print("CndTemp: ");
  Serial.println(U_ccmList[CCMID_CndInAirHumid].value);

//湿度　温風機が動く
  showValueHumidity = U_ccmList[CCMID_InAirHumid].value;
  if(setONOFFAUTO_Humidity==0) {
    U_ccmList[CCMID_CndInAirHumid].value=0;
    Serial.println("HUMID-OFF");
    digitalWrite(RLY1, HIGH);
 }//Manual OFF
  else if(setONOFFAUTO_Humidity==1) {
    U_ccmList[CCMID_CndInAirHumid].value=1;
    Serial.println("HUMID-ON");
    digitalWrite(RLY1, LOW);
  }//Manual ON
  else if(setONOFFAUTO_Humidity==2 && U_ccmList[CCMID_InAirHumid].validity && U_ccmList[CCMID_InAirHumid].value > setONHumidityFromWeb) {
    U_ccmList[CCMID_CndInAirHumid].value=1;
    Serial.println("HUMID-ON");
    digitalWrite(RLY1, LOW);
  }//Auto ON
  else {
    U_ccmList[CCMID_CndInAirHumid].value=0;
    Serial.println("HUMID-OFF");
    digitalWrite(RLY1, HIGH);
  }//OFF
  showValueStatusHumidity = U_ccmList[CCMID_CndInAirHumid].value;
  Serial.print("CndHumid: ");
  Serial.println(showValueStatusHumidity);

//照度　LEDランプ
  showValueIlluminance = U_ccmList[CCMID_Illuminance].value;
  if(setONOFFAUTO_Illuminance==0) {
    U_ccmList[CCMID_CndIlluminance].value=0;
    Serial.println("ILLUMINANCE-OFF");
    led_lamp(0);
  }//Manual OFF
  else if(setONOFFAUTO_Illuminance==1) {
    U_ccmList[CCMID_CndIlluminance].value=1;
    Serial.println("ILLUMINANCE-ON");
    led_lamp(1);
  }//Manual ON
  else if(setONOFFAUTO_Illuminance==2 && U_ccmList[CCMID_Illuminance].validity && U_ccmList[CCMID_Illuminance].value<setONIlluminanceFromWeb) {
    U_ccmList[CCMID_CndIlluminance].value=1;
    Serial.println("ILLUMINANCE-ON");
    led_lamp(1);
  }//Auto ON
  else {
    U_ccmList[CCMID_CndIlluminance].value=0;
    Serial.println("ILLUMINANCE-OFF");
    led_lamp(0);
  }//OFF
  showValueStatusIlluminance = U_ccmList[CCMID_CndIlluminance].value;
  Serial.print("CndIlluminance: ");
  Serial.println(showValueStatusIlluminance);

//可変抵抗器　そくそう
  showValueREncoder = U_ccmList[CCMID_REncoder].value;
  if(setONOFFAUTO_REncoder==2 && U_ccmList[CCMID_REncoder].validity ) {
    if (U_ccmList[CCMID_REncoder].value <  300) {
      U_ccmList[CCMID_CndREncoder].value=1;
      Serial.println("REncoder-Under 300");
      sidewindow(1);
    } else if (U_ccmList[CCMID_REncoder].value >  700) {
      U_ccmList[CCMID_CndREncoder].value=2;
      Serial.println("REncoder-Over 700");
      sidewindow(2);
    } else {
      U_ccmList[CCMID_CndREncoder].value=4;
      Serial.println("REncoder-STOP");
      sidewindow(0);
    }
  }
  showValueStatusREncoder = U_ccmList[CCMID_CndREncoder].value;
  Serial.print("CndREncoder: ");
  Serial.println(showValueStatusREncoder);

}


//
// Side Window Motor Contorol
//
//   m : running mode
//       0 = STOP
//       1 = CLOSE
//       2 = OPEN
//       OTHERS = STOP
//
void sidewindow(int m) {
  switch(m) {
  case 1:
    digitalWrite(RLY7,LOW);
    digitalWrite(RLY8,HIGH);
    lcdmenu |= 0x04;
    break;
  case 2:
    digitalWrite(RLY7,HIGH);
    digitalWrite(RLY8,LOW);
    lcdmenu |= 0x02;
    break;
  default:
    digitalWrite(RLY7,HIGH);
    digitalWrite(RLY8,HIGH);
    lcdmenu &= 0x01;
  }
}

//
//  LED light on/off
//
void led_lamp(int m) {
  switch(m) {
  case 0:
    digitalWrite(RLY3,HIGH);
    lcdmenu &= 0xfe;
    break;
  default:
    digitalWrite(RLY3,LOW);
    lcdmenu |= 0x01;
    break;
  }
}

void progmem2lcdtxt(char ptxt[]) {
  int k;
  for(k=0;k<20;k++) {
    lcdtxt[k]=pgm_read_byte(&ptxt[k]);
    if (lcdtxt[k]==(char)0) return;
  }
}

void clrLcdScr(void) {
  int x,y;
  for(y=0;y<4;y++) {
    for(x=0;x<20;x++) {
      lcdscr[y][x] = (char)0;
    }
  }
}

void makeScr(int menu) {
  int i,x;
  byte ipa[4];

  progmem2lcdtxt(U_name);
  for(x=0;x<20;x++) {
    lcdscr[0][x] = lcdtxt[x];
    if (lcdtxt[x]==(char)0) break;
  }
  strcpy(&lcdscr[1][0],"192.168.11.100/24");

  switch(menu) {
  case 1:
    strcpy(&lcdscr[2][0],"Side Window STOP    ");
    strcpy(&lcdscr[3][0],"LED Lamp ON         ");
    break;
  case 2:
    strcpy(&lcdscr[2][0],"Side Window OPEN    ");
    strcpy(&lcdscr[3][0],"LED Lamp OFF        ");
    break;
  case 3:
    strcpy(&lcdscr[2][0],"Side Window OPEN    ");
    strcpy(&lcdscr[3][0],"LED Lamp ON         ");
    break;
  case 4:
    strcpy(&lcdscr[2][0],"Side Window CLOSE   ");
    strcpy(&lcdscr[3][0],"LED Lamp OFF        ");
    break;
  case 5:
    strcpy(&lcdscr[2][0],"Side Window CLOSE   ");
    strcpy(&lcdscr[3][0],"LED Lamp ON         ");
    break;		   
  default:
    strcpy(&lcdscr[2][0],"Agribusiness        ");
    strcpy(&lcdscr[3][0],"  Creation Fair 2023");
  }
}

void dispScr(int menu) {
  int x,y;
  makeScr(menu);
  for(y=0;y<4;y++) {
    lcd.setCursor(0,y);
    lcd.print(&lcdscr[y][0]);
  }
}
