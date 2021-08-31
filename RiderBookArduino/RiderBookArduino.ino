#include "DFRobotDFPlayerMini.h"
#include <SoftwareSerial.h>

#define DF_RX_PIN      2
#define DF_TX_PIN      3

SoftwareSerial Serial1(DF_RX_PIN, DF_TX_PIN);  //RX  TX
DFRobotDFPlayerMini mp3_player;

//整个循环的等待时间，避免重复触发。
#define LOOP_DELAY_MS 10

#define SIDE_SW_PIN     4
#define BOTTOM_SW_PIN   5

#define ON  LOW
#define OFF HIGH

#define OPEN  LOW
#define CLOSE HIGH

#define STATE_NORMAL                     0 //普通状态
#define STATE_PAGE_PUSH                  1 //按下书页播放的介绍
#define STATE_BUTTOM_WAIT                2 //按下书页进入等待
#define STATE_BUTTOM_TITLE               3 //快速按下放开播放书名
#define STATE_BUTTOM_DRIVER_PUSH         4 //插入
#define STATE_BUTTOM_DRIVER_RELEASE      5 //拔出
#define STATE_BUTTOM_DRIVER_HENSHIN      6 //变身
#define STATE_BUTTOM_DRIVER_PUSH_LOOP    7 //插入，待机

bool PUSH_HENSHIN = false; //是否播放变身声音，标准板子不支持
bool PUSH_LOOP = false;    //插入后是否播放待机，标准板子不支持
#define PUSH_LOOP_DELAY_MS 10 //插入后播放待机的延迟，标准板子不支持

//之前的状态机存储
uint8_t bottom_sw        = OFF;
uint8_t side_sw          = OFF;
uint8_t prev_bottom_sw   = OFF;
uint8_t prev_side_sw     = OFF;
uint8_t state            = STATE_NORMAL;
uint8_t prev_state       = STATE_NORMAL;

void printDetail(uint8_t type, int value);

void setup(void) {
  Serial.begin(115200);
  Serial.println("Link Start!");
  processDFP();
  //mp3_player.playFolder(0, 1);

  pinMode(BOTTOM_SW_PIN,   INPUT_PULLUP);
  pinMode(SIDE_SW_PIN,     INPUT_PULLUP);
}

bool needPlay = false;
long last_ms = 0;
bool isSleeping = false;
long last_ms_Sleep = 0;

void loop() {
  unsigned long now_ms = millis();

  //获取当前状态
  bottom_sw   = digitalRead(BOTTOM_SW_PIN);
  side_sw     = digitalRead(SIDE_SW_PIN);

  //开始状态判断，先判断之前的状态，再判断之后的状态
  switch (prev_state) {
    //之前是无状态
    case STATE_NORMAL:
      if (bottom_sw == ON) {
        last_ms = now_ms;
        state = STATE_BUTTOM_WAIT;
      }
      if (side_sw == ON) {
        state = STATE_PAGE_PUSH;
        needPlay = true;
      }
      break;
    case STATE_PAGE_PUSH:
      if (side_sw == OFF) {
        state = STATE_NORMAL;
      }
      break;
    case STATE_BUTTOM_WAIT:
      if (now_ms - last_ms > 200) {
        if (bottom_sw == ON) {
          state = STATE_BUTTOM_DRIVER_PUSH;
          needPlay = true;
        } else {
          state = STATE_BUTTOM_TITLE;
          needPlay = true;
        }
      }
      break;
    case STATE_BUTTOM_TITLE:
      state = STATE_NORMAL;
      break;
    case STATE_BUTTOM_DRIVER_PUSH:
      if (PUSH_LOOP) {
        state = STATE_BUTTOM_DRIVER_PUSH_LOOP;
        needPlay = true;
      }
      if (bottom_sw == OFF) {
        state = STATE_BUTTOM_DRIVER_RELEASE;
        needPlay = true;
      }
      if (side_sw == ON) {
        state = STATE_BUTTOM_DRIVER_HENSHIN;
        needPlay = true;
      }
      break;
    case STATE_BUTTOM_DRIVER_RELEASE:
      state = STATE_NORMAL;
      break;
    case STATE_BUTTOM_DRIVER_HENSHIN:
      if (bottom_sw == OFF) {
        state = STATE_BUTTOM_DRIVER_RELEASE;
        needPlay = true;
      }
      break;
    case STATE_BUTTOM_DRIVER_PUSH_LOOP:
      if (side_sw == ON) {
        state = STATE_BUTTOM_DRIVER_HENSHIN;
        needPlay = true;
      }
      break;
  }

  print_state();
  play_music();


  if (prev_state != state) {
    Serial.println("Update Sleep MS!");
    last_ms_Sleep = now_ms;
    isSleeping = false;
  }

  if (!isSleeping && now_ms - last_ms_Sleep > 1000 * 60 * 5) {
    Serial.println("Sleep!");
    mp3_player.sleep();
    isSleeping = true;
  }

  //保存之前的状态机
  prev_bottom_sw   = bottom_sw;
  prev_side_sw     = side_sw;
  prev_state       = state;
  delay(10);
}

/*
  #define SOUND_SIDE_PAGE               "/0.mp3"
  #define SOUND_BUTTOM_TITLE            "/1.mp3"
  #define SOUND_BUTTOM_DRIVER_PUSH      "/2.mp3"
  #define SOUND_BUTTOM_DRIVER_RELEASE   "/3.mp3"
  #define SOUND_BUTTOM_DRIVER_HENSHIN   "/4.mp3"
  #define SOUND_BUTTOM_DRIVER_PUSH_LOOP "/5.mp3"
*/
#define SOUND_SIDE_PAGE               0
#define SOUND_BUTTOM_TITLE            1
#define SOUND_BUTTOM_DRIVER_PUSH      2
#define SOUND_BUTTOM_DRIVER_RELEASE   3
#define SOUND_BUTTOM_DRIVER_HENSHIN   4
#define SOUND_BUTTOM_DRIVER_PUSH_LOOP 5

void play_music() {
  if (needPlay) {
    needPlay = false;
    switch (state) {
      case STATE_PAGE_PUSH:
        play_sound(SOUND_SIDE_PAGE);
        break;
      case STATE_BUTTOM_TITLE:
        play_sound(SOUND_BUTTOM_TITLE);
        break;
      case STATE_BUTTOM_DRIVER_PUSH:
        play_sound(SOUND_BUTTOM_DRIVER_PUSH);
        break;
      case STATE_BUTTOM_DRIVER_RELEASE:
        play_sound(SOUND_BUTTOM_DRIVER_RELEASE);
        break;
      case STATE_BUTTOM_DRIVER_HENSHIN:
        if (PUSH_HENSHIN) {
          play_sound(SOUND_BUTTOM_DRIVER_HENSHIN);
        }
        break;
      case STATE_BUTTOM_DRIVER_PUSH_LOOP:
        if (PUSH_LOOP) {
          play_sound(SOUND_BUTTOM_DRIVER_PUSH_LOOP);
        }
        break;
    }
  }
}

void play_sound(int sound_status) {
  Serial.print("Playing: ");
  Serial.println(sound_status);
  mp3_player.playFolder(0, sound_status);
  //DF1201S.playSpecFile(sound_status);
}

void print_state() {
  if (prev_state != state) {
    switch (state) {
      case STATE_NORMAL:                      Serial.println(F("STATE_NORMAL"));                    break;
      case STATE_PAGE_PUSH:                   Serial.println(F("STATE_PAGE_PUSH"));                 break;
      case STATE_BUTTOM_WAIT:                 Serial.println(F("STATE_BUTTOM_WAIT"));               break;
      case STATE_BUTTOM_TITLE:                Serial.println(F("STATE_BUTTOM_TITLE"));              break;
      case STATE_BUTTOM_DRIVER_PUSH:          Serial.println(F("STATE_BUTTOM_DRIVER_PUSH"));        break;
      case STATE_BUTTOM_DRIVER_RELEASE:       Serial.println(F("STATE_BUTTOM_DRIVER_RELEASE"));     break;
      case STATE_BUTTOM_DRIVER_HENSHIN:       Serial.println(F("STATE_BUTTOM_DRIVER_HENSHIN"));     break;
      case STATE_BUTTOM_DRIVER_PUSH_LOOP:     Serial.println(F("STATE_BUTTOM_DRIVER_PUSH_LOOP"));   break;
      default: ;
    }
  }
}

void processDFP() {
  Serial1.begin(9600);

  if (!mp3_player.begin(Serial1, true, false)) {
    Serial.println("初始化失败，请检查接线！");
    while (true);
  }

  Serial.println("Loaded!");
  mp3_player.volume(25);
  Serial.println(mp3_player.readVolume());
  mp3_player.enableDAC();
  //mp3_player.randomAll();

  Serial.println(mp3_player.readState());

  if (mp3_player.available()) {
    printDetail(mp3_player.readType(), mp3_player.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }


}

void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      Serial.println(F("Unknown"));
      break;
  }
}
