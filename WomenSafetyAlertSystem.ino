#include <SoftwareSerial>
#include <LowPower>
#include <TinyGPS++>
#include <Adafruit_Fingerprint>

SoftwareSerial gpsSerial(4,5);
SoftwareSerial gsmSerial(3,2);
SoftwareSerial fingerSerial(10,11);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);
TinyGPSPlus gps;
unsigned long start;
uint8_t s;
int count = 0;

void setup() {
    Serial.begin(9600);
    fingerSerial.begin(57600);
    finger.getTemplateCount();
    Serial.println(finger.templateCount);
    if (finger.templateCount == 0) {
        do {
            s = FingerprintEnroll(1);
        }
        while(s!=FINGERPRINT_OK);
        Serial.println("Finger 1 Enrolled");
        delay(3000);
        do {
            s = FingerprintEnroll(2);
        }
        while(s!=FINGERPRINT_OK);
        Serial.println("Finger 2 Enrolled");
    }
    start = millis();
    }

void loop() {
    double latitude=0,longitude=0;
    unsigned long current = millis();
    while ((current-start) > 60000) {
        int x = VerifyFingerprint();

        if (x==1) {
            Serial.println("Finger Accepted");
            start = current;
            current = millis();
            count = 0;
            continue;
        }
        else if (x==2) {
            clearInputs();
            delay(10000);
            Serial.println("Arduino Sleep");
            LowPower.powerDown(SLEEP_FOREVER,ADC_OFF,BOD_OFF);
        }
        else {
            count++;
            Serial.println("error");
        }
        if (count > 900) {
            clearInputs();
            Serial.println("Danger");
            fingerSerial.end();
            gpsSerial.begin(9600);
            do {
                if (gpsSerial.available()) {
                    if (gps.encode(gpsSerial.read())) {
                        if (gps.location.isValid()){
                            latitude = gps.location.lat();
                            longitude = gps.location.lng();
                        }
                    }
                }
            }
            while(latitude==0 && longitude==0);
            gpsSerial.end();
            gsmSerial.begin(9600);
            gsmSerial.println("AT+CMGF=1");
            gsmSerial.println("AT+CMGS=\"+919603789154\"\r");
            String warning = "Your Daughter is in danger, Please Help her, She's at "+String(latitude,3)+" , "+String(longitude,3);
            gsmSerial.println(warning);
            Serial.println(warning);
            gsmSerial.println("ATD+919603789154;");
            Serial.println("Alert Sent");
            delay(10000);
            Serial.println("Arduino Sleep");
            LowPower.powerDown(SLEEP_FOREVER,ADC_OFF,BOD_OFF);
        }
    }
}

uint8_t FingerprintEnroll(uint8_t id) {
    uint8_t p = -1;
    do {
        p = finger.getImage();
    }

    while (p != FINGERPRINT_OK);
    p = finger.image2Tz(1);
    p = 0;
    while (p != FINGERPRINT_NOFINGER){
        p = finger.getImage();
    }
    p = -1;
    do {
        p = finger.getImage();
    }
    while (p != FINGERPRINT_OK);
    p = finger.image2Tz(2);
    p = finger.createModel();
    p = finger.storeModel(id);
    return p;
}

uint8_t VerifyFingerprint() {
    uint8_t p = finger.getImage();
    if (p != FINGERPRINT_OK) return -1;
    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) return -1;
    p = finger.fingerFastSearch();
    if (p != FINGERPRINT_OK) return -1;
    return finger.fingerID;
}

uint8_t deleteFingerprint(uint8_t id) {
    uint8_t p = finger.deleteModel(id);
    if (p != FINGERPRINT_OK) return -1;
    return p;
}

void clearInputs() {
    do {
        s = deleteFingerprint(1);
    }
    while (s!=FINGERPRINT_OK);
    Serial.println(" Finger 1 deleted ");
    do {
        s = deleteFingerprint(2);
    }
    while (s!=FINGERPRINT_OK);
    Serial.println(" Finger 2 deleted ");
}