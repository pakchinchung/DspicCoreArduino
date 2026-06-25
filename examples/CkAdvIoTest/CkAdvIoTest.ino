/* CkAdvIoTest (dsPIC33CK256MP508) - cross-family tone()+pulseIn() loopback on RE6. */
#define PIN RE6
bool chk(unsigned int f){
  tone(PIN,f); delay(20);
  unsigned long hi=pulseIn(PIN,HIGH,50000UL), lo=pulseIn(PIN,LOW,50000UL);
  unsigned long per=hi+lo, m=per?1000000UL/per:0;
  Serial.print("tone "); Serial.print(f); Serial.print("Hz -> "); Serial.print(m);
  Serial.print("Hz (HIGH="); Serial.print(hi); Serial.print(" LOW="); Serial.print(lo); Serial.println(")");
  unsigned long d=m>f?m-f:f-m; return d*100<f*8UL;
}
void setup(){ Serial.begin(9600); delay(50); Serial.println("=== CK tone+pulseIn ==="); }
void loop(){
  bool ok=chk(1000); ok&=chk(2000);
  noTone(PIN); delay(10);
  unsigned long after=pulseIn(PIN,HIGH,20000UL);
  Serial.print("after noTone: "); Serial.print(after); Serial.println("us (0)");
  Serial.print("RESULT: "); Serial.println((ok&&after==0)?"PASS":"CHECK");
  Serial.println(); delay(3000);
}
