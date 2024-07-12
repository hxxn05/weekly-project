#include <Wire.h>           // I2C 통신을 위한 Wire 라이브러리 포함
#include <NewPing.h>        // 초음파 센서 거리 측정을 위한 NewPing 라이브러리 포함
#include <MsTimer2.h>       // MsTimer2 라이브러리 포함

#define SONAR_NUM 3         // 초음파 센서의 개수
#define MAX_DISTANCE 100    // 초음파 센서의 측정 최대 거리

#define SIZE 5              // 재귀 이동 평균 필터를 위한 배열 크기
#define sensorPin A0        // 가변저항 연결 핀 번호

// 초음파 센서 인덱스 정의
#define Front 0            
#define Left  1 
#define Right 2

// 초음파 센서 핀 번호 정의
#define F_TRIG 12  // 앞쪽 초음파 센서의 트리거 핀
#define F_ECHO 13  // 앞쪽 초음파 센서의 에코 핀
#define L_TRIG 16  // 왼쪽 초음파 센서의 트리거 핀
#define L_ECHO 17  // 왼쪽 초음파 센서의 에코 핀
#define R_TRIG 14  // 오른쪽 초음파 센서의 트리거 핀
#define R_ECHO 15  // 오른쪽 초음파 센서의 에코 핀

// 초음파 센서 객체 생성
NewPing sonar[SONAR_NUM] = 
{   
  NewPing(F_TRIG, F_ECHO, MAX_DISTANCE), 
  NewPing(L_TRIG, L_ECHO, MAX_DISTANCE),
  NewPing(R_TRIG, R_ECHO, MAX_DISTANCE)
};

// 각 센서 데이터 배열 선언 및 초기화
float sensorData[SONAR_NUM][SIZE] = {{0.0}}; // 초음파 센서 데이터를 저장할 2차원 배열
float resistanceData[SIZE] = {0.0}; // 가변 저항 데이터를 저장할 배열

//////////////////// 재귀 이동 평균 필터 적용 함수 ///////////////////////////////
float recursive_moving_average(float* data, float new_value) 
{
  static float avg = 0.0; // 평균 값을 저장할 정적 변수 선언 및 초기화

  // 기존 배열의 값을 한 칸씩 앞으로 이동
  for (int i = 0; i < SIZE - 1; i++) 
  {
    data[i] = data[i + 1];
  }

  data[SIZE - 1] = new_value; // 배열의 마지막에 새로운 값을 추가

  // 재귀 이동 평균 필터 공식 적용
  avg = avg + (data[SIZE - 1] - data[0]) / (float)SIZE; 

  return avg; // 계산된 평균값 반환
}

float result = 0.0; // 전역 변수로 선언, 가변 저항 값을 저장

// 가변저항 값을 읽어 재귀 이동 평균 필터 계산 및 출력하는 함수 (Mstimer2를 사용해서 100ms마다 호출)
void Read_resistance(void) 
{
   float new_value = analogRead(sensorPin); // A0 핀에서 읽은 가변 저항 값을 변수에 저장
   result = recursive_moving_average(resistanceData, new_value); // 가변 저항 재귀 이동 평균 필터 적용 후 결과 저장
   Serial.print("가변저항: "); Serial.println(result); // 필터링된 가변 저항 값을 시리얼 모니터에 출력
   Serial.println();
}

void setup() 
{
  // 초음파 센서 핀을 출력/입력으로 설정
  pinMode(F_TRIG, OUTPUT);
  pinMode(F_ECHO, INPUT);
  pinMode(L_TRIG, OUTPUT);
  pinMode(L_ECHO, INPUT);
  pinMode(R_TRIG, OUTPUT);
  pinMode(R_ECHO, INPUT);
  
  Serial.begin(115200); // 시리얼 통신 시작

  MsTimer2::set(100, Read_resistance); // 100ms마다 Read_resistance 함수 호출 설정
  MsTimer2::start(); // MsTimer2 타이머 시작

  Wire.begin(); // I2C 통신 시작
}

void loop() 
{
  float sonar_values[SONAR_NUM]; // 초음파 센서 측정값을 저장할 배열

  // 각 초음파 센서로부터 거리 측정
  for (int i = 0; i < SONAR_NUM; i++) {
    sonar_values[i] = sonar[i].ping_cm(); // 각 초음파 센서로 측정된 거리를 cm 단위로 반환
    if (sonar_values[i] == 0.0) sonar_values[i] = MAX_DISTANCE; // 측정되지 않은 경우 최대 거리 값으로 설정
  }
  
  // 재귀 이동 평균 필터 함수 호출 후 반환값을 실수형으로 변수에 저장
  float filtered_values[SONAR_NUM];
  for (int i = 0; i < SONAR_NUM; i++) {
    filtered_values[i] = recursive_moving_average(sensorData[i], sonar_values[i]);
  }

  // 필터링된 결과값을 시리얼 모니터에 출력
  Serial.print("정면: "); Serial.print(filtered_values[Front]); Serial.print("cm ");
  Serial.print("왼쪽: "); Serial.print(filtered_values[Left]); Serial.print("cm ");
  Serial.print("오른쪽: "); Serial.print(filtered_values[Right]); Serial.println("cm ");

  // 공용체 정의 및 변수 선언 : 4바이트 전송 (초음파 센서 3개 및 가변 저항 재귀 평균 이동 필터)
  union {
    float value;
    byte bytes[4];
  } sensorUnion[SONAR_NUM], resistanceUnion;

  // 각 초음파 센서 필터링된 값을 공용체에 저장
  for (int i = 0; i < SONAR_NUM; i++) {
    sensorUnion[i].value = filtered_values[i];
  }
  resistanceUnion.value = result; // 가변 저항 필터링된 값을 공용체에 저장

  // 슬레이브 보드로 데이터 전송
  Wire.beginTransmission(8); // 슬레이브 주소 8번
  for (int i = 0; i < SONAR_NUM; i++) {
    Wire.write(sensorUnion[i].bytes, 4); // 초음파 센서 값 전송
  }
  Wire.write(resistanceUnion.bytes, 4); // 가변 저항 값 전송
  Wire.endTransmission(); // 데이터 송신 종료

  delay(100); // 100ms 대기
}
