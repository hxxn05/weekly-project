#include <Wire.h> // I2C 통신을 위한 Wire 라이브러리 포함

// float 값을 byte 배열로 저장할 수 있는 공용체 정의
union Float_Union 
{
  float value;      // float형 멤버 
  byte bytes[4];    // float 값을 byte로 저장하는 배열
};

// 센서 데이터를 저장할 공용체 변수들 선언
Float_Union F_Union, L_Union, R_Union, Result_Union;

void setup() 
{
  Serial.begin(115200);         // 시리얼 통신 시작, 보드레이트 115200
  Wire.begin(8);                // I2C 슬레이브 주소 8번으로 설정
  Wire.onReceive(receiveEvent); // I2C 데이터 수신 시 호출될 이벤트 핸들러 함수 등록
}

void loop()
{
  delay(100); // 100ms 대기, 루프를 빠르게 도는 것을 방지
}

// I2C 데이터 수신 시 호출되는 함수
void receiveEvent(int howMany)   // howMany: 마스터 보드로부터 수신된 바이트의 개수
{
  if (Wire.available() >= 16) // 모든 데이터(16개의 바이트)가 수신될 때까지 대기
  { 
    readData(F_Union);    // 정면 센서 데이터 읽기
    readData(L_Union);     // 왼쪽 센서 데이터 읽기
    readData(R_Union);    // 오른쪽 센서 데이터 읽기
    readData(Result_Union);   // 가변 저항 데이터 읽기
    
    printData(); // 수신된 데이터 시리얼 모니터에 출력
  }
}

// 공용체 데이터를 읽는 함수
void readData(Float_Union &dataUnion)
{
  for (int i = 0; i < 4; i++) 
  {
    dataUnion.bytes[i] = Wire.read(); // I2C 버퍼에서 데이터를 읽어 공용체의 byte 배열에 저장
  }
}

// 수신된 데이터를 시리얼 모니터에 출력하는 함수
void printData()
{
  Serial.print("정면: "); Serial.print(F_Union.value); Serial.print(" cm, ");  // 정면 센서 데이터 출력
  Serial.print("왼쪽: "); Serial.print(L_Union.value); Serial.print(" cm, ");   // 왼쪽 센서 데이터 출력
  Serial.print("오른쪽: "); Serial.print(R_Union.value); Serial.print(" cm, "); // 오른쪽 센서 데이터 출력
  Serial.print("가변저항 값: "); Serial.print(Result_Union.value); Serial.println(" "); // 가변 저항 데이터 출력
}
