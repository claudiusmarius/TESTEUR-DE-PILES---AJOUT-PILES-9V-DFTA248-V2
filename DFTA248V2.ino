// Testeur de piles AA et AAA + 9V
// Fonctionne le 21/02/2026 13H33
// Auteur Claude DUFOURMONT

#include <Adafruit_NeoPixel.h>

// ------------------------------------------------------------
#define PIN_LED     0
#define PIN_CHARGE  1
#define PIN_VBAT    A1
#define PIN_RSENSE  A3
#define PIN_BUZZER  4

#define N_PIXELS 12
#define RSENSE   2.2f

Adafruit_NeoPixel strip(N_PIXELS, PIN_LED, NEO_GRB + NEO_KHZ800);

// ------------------------------------------------------------
//  CALIBRATIONS
// ------------------------------------------------------------
const float VREF_3V3 = 3.313f;

float K_DIV1 = 1.000f;   // AA / AAA

float VBAT9_mesuree = 9.025f;
float VADC9_mesuree = 2.917f;

// Coefficient réel 9V
float K_DIV2 = VADC9_mesuree / VBAT9_mesuree;

float K_DIV = 1.0f;
bool mode9V = false;

// ------------------------------------------------------------
//  SEUILS AA / AAA
// ------------------------------------------------------------
float seuil_TresBas_vide_AA      = 1.10;
float seuil_Acceptable_vide_AA   = 1.36;
float seuil_TresBas_charge_AA    = 1.10;
float seuil_Acceptable_charge_AA = 1.36;

// ------------------------------------------------------------
//  SEUILS 9V SPÉCIFIQUES
// ------------------------------------------------------------
float seuil_TresBas_vide_9V      = 6.5;
float seuil_Acceptable_vide_9V   = 7.5;
float seuil_TresBas_charge_9V    = 6.5;
float seuil_Acceptable_charge_9V = 7.5;

// Seuils actifs utilisés par le programme
float seuil_TresBas_vide;
float seuil_Acceptable_vide;
float seuil_TresBas_charge;
float seuil_Acceptable_charge;

// ============================================================
// ADC STABLE
// ============================================================
uint16_t readADC_stable(uint8_t pin) {
  analogRead(pin);
  delayMicroseconds(250);
  return analogRead(pin);
}

// ============================================================
// CONVERSION ADC → TENSION
// ============================================================
float lireVBAT() {
  float raw = readADC_stable(PIN_VBAT);
  float V = raw * VREF_3V3 / 1023.0;
  return V / K_DIV;
}

// ============================================================
// MESURE MULTIPLE
// ============================================================
bool mesurerStabilite(float *moy, float seuil) {

  const uint8_t N = 6;
  float M[N];

  for (uint8_t i = 0; i < N; i++) {
    M[i] = lireVBAT();
    delay(80);
  }

  float sum = 0, minV = M[0], maxV = M[0];

  for (uint8_t i = 0; i < N; i++) {
    sum += M[i];
    if (M[i] < minV) minV = M[i];
    if (M[i] > maxV) maxV = M[i];
  }

  *moy = sum / N;
  return ((maxV - minV) < seuil);
}

// ============================================================
// CLASSE COULEUR
// ============================================================
int classerPile(float V, float s1, float s2) {
  if (V < s1) return 0;
  if (V < s2) return 1;
  return 2;
}

// ============================================================
// BUZZER
// ============================================================
void bipBuzzer(int th, int tl, int n) {
  for (int i = 0; i < n; i++) {
    digitalWrite(PIN_BUZZER, HIGH); delay(th);
    digitalWrite(PIN_BUZZER, LOW);  delay(tl);
  }
}

// ============================================================
// SETUP = ONE SHOT
// ============================================================
void setup() {

  strip.begin();
  strip.clear();
  strip.show();

  pinMode(PIN_CHARGE, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_CHARGE, LOW);

  bipBuzzer(200, 100, 3);
  bipBuzzer(600, 100, 1);
  delay(800);

  // ============================================================
  // DÉTECTION TYPE DE PILE (ADC BRUT)
  // ============================================================

  uint16_t rawTest = readADC_stable(PIN_VBAT);
  float Vadc = rawTest * VREF_3V3 / 1023.0;

  // Si ADC > 1.9V → forcément 9V
  if (Vadc > 1.9f) {
    mode9V = true;
    K_DIV = K_DIV2;
  } else {
    mode9V = false;
    K_DIV = K_DIV1;
  }

  // Sélection des seuils selon type détecté
  if (mode9V) {
    seuil_TresBas_vide      = seuil_TresBas_vide_9V;
    seuil_Acceptable_vide   = seuil_Acceptable_vide_9V;
    seuil_TresBas_charge    = seuil_TresBas_charge_9V;
    seuil_Acceptable_charge = seuil_Acceptable_charge_9V;
  } else {
    seuil_TresBas_vide      = seuil_TresBas_vide_AA;
    seuil_Acceptable_vide   = seuil_Acceptable_vide_AA;
    seuil_TresBas_charge    = seuil_TresBas_charge_AA;
    seuil_Acceptable_charge = seuil_Acceptable_charge_AA;
  }

  float Vinv = lireVBAT();

  // ============================================================
  // INVERSION OU ABSENCE
  // ============================================================
  if (Vinv < 0.05f) {

    for (uint8_t k = 0; k < 40; k++) {
      strip.setPixelColor(3,  strip.Color(200,0,0));
      strip.setPixelColor(7,  strip.Color(200,0,0));
      strip.setPixelColor(11, strip.Color(200,0,0));
      strip.show();
      delay(50);

      strip.clear();
      strip.show();
      delay(50);
    }
    return;
  }

  // ============================================================
  // 1) MESURE À VIDE
  // ============================================================
  delay(3000);

  float Vvide;
  bool stableV = mesurerStabilite(&Vvide, 0.015);
  int nv = classerPile(Vvide, seuil_TresBas_vide, seuil_Acceptable_vide);

  uint32_t colV =
      (nv==0) ? strip.Color(200,0,0) :
      (nv==1) ? strip.Color(255,120,0) :
                strip.Color(0,200,0);

  // 🔊 BIP 1
  bipBuzzer(80,50,1);

  strip.setPixelColor(nv, colV);
  strip.setPixelColor(3,
      stableV ? strip.Color(0,40,0)
              : strip.Color(0,0,200));
  strip.show();

  delay(400);

  // ============================================================
  // 2) MESURE EN CHARGE
  // ============================================================
  digitalWrite(PIN_CHARGE, HIGH);
  delay(4000);

  // 🔊 BIP 2
  bipBuzzer(80,50,2);

  float Vcharge;
  bool stableC = mesurerStabilite(&Vcharge, 0.015);

  float Vrs = readADC_stable(PIN_RSENSE) * VREF_3V3 / 1023.0;
  float I = Vrs / RSENSE;

  digitalWrite(PIN_CHARGE, LOW);

  if (I > 0.20f) {
    strip.fill(strip.Color(200,0,0));
    strip.show();
    return;
  }

  int nc = classerPile(Vcharge, seuil_TresBas_charge, seuil_Acceptable_charge);

  uint32_t colC =
      (nc==0) ? strip.Color(200,0,0) :
      (nc==1) ? strip.Color(255,120,0) :
                strip.Color(0,200,0);

  strip.setPixelColor(4 + nc, colC);
  strip.setPixelColor(7,
      stableC ? strip.Color(0,40,0)
              : strip.Color(0,0,200));
  strip.show();

  // ============================================================
  // 3) DIAGNOSTIC FINAL
  // ============================================================
  delay(3000);

  // 🔊 BIP 3
  bipBuzzer(80,50,3);

  float deltaV = Vvide - Vcharge;

  float seuil_surtension = mode9V ? 9.9f : 1.65f;

  if (Vvide > seuil_surtension) {

    for (int i = 0; i < 12; i++) {
      strip.setPixelColor(11, strip.Color(0,0,200));
      strip.show();
      delay(100);
      strip.setPixelColor(11, strip.Color(200,0,0));
      strip.show();
      delay(100);
    }

    strip.setPixelColor(11, strip.Color(200,0,0));
    strip.show();
  }
  else {

    if (nv==2 && nc==2 && deltaV < (mode9V ? 1.5f : 0.25f))
      strip.setPixelColor(10, strip.Color(0,200,0));

    else if (nv>0 && nc>0 && deltaV < (mode9V ? 1.8f : 0.30f))
      strip.setPixelColor(9, strip.Color(255,120,0));

    else
      strip.setPixelColor(8, strip.Color(200,0,0));

    strip.show();
  }
}

void loop() {
}