void setup() {
  // put your setup code here, to run once:
  int k=0;
}

void loop() {
  // put your main code here, to run repeatedly:
  int k = analogRead(38);
  Serial.print(k);
  Serial.print("\n");
  delay(100);
}
