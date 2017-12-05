#include <EEPROM.h>
#include <Keypad.h>
#include <UIPEthernet.h>


// Instalar via Library Manager da IDE
#include <ArduinoHttpClient.h>

// Alterar o último valor para o id do seu kit
const byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x08 };
EthernetClient ethclient;

HttpClient client = HttpClient(ethclient, "192.168.3.186", 3000);

#define SMS_TWILIO_SID "AC939ddc5ffe38b2b231991cdb9aaa0e27"
#define SMS_TWILIO_TOKEN "09c61e72ef221c98ec73c5c183d93ef2"
#define SMS_PHONE_TO "5511988795656"
#define SMS_PHONE_FROM "14842658133"
#define SMS_MESSAGE "Invasor na casa"
#define CONTENT_TYPE "application/x-www-form-urlencoded"

const char* parametros = "sid=" SMS_TWILIO_SID "&token=" SMS_TWILIO_TOKEN "&to=" SMS_PHONE_TO "&from=" SMS_PHONE_FROM "&body=" SMS_MESSAGE;

#define RESPONSE_SIZE 60
char response[RESPONSE_SIZE] = {};

int sensor = 2; //sensor digital
int tempo;
long ultimoTempo;
bool contando;

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

bool digitandoSenha = false;
String senha;
String senhaFixa = "1577";

byte rowPins[ROWS] = {A0, A1, A2, A3}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 4, 3}; //connect to the column pinouts of the keypad

boolean entry = false;

const int LED_ALERT = 7;
const int LED_SAFE = 6;

bool alarmeAtivado = false;

const int counter;

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );


//=============================================================================


// C runtime variables
// -------------------
extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

/*!
  @function   freeMemory
  @abstract   Return available RAM memory
  @discussion This routine returns the ammount of RAM memory available after
  initialising the C runtime.
  @param
  @return     Free RAM available.
*/
static int freeMemory ( void )
{
  int free_memory;

  if ((int)__brkval == 0)
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  else
    free_memory = ((int)&free_memory) - ((int)__brkval);

  return free_memory;
}

//==============================================================================
void setup() {
  Serial.begin(9600);

  if (Ethernet.begin(mac)) {
    Serial.println(F("Conectado via DHCP"));
    Serial.print(F("IP recebido:")); Serial.println(Ethernet.localIP());
  }
  pinMode(sensor, INPUT_PULLUP); //sensor

  pinMode(LED_ALERT, OUTPUT);
  pinMode(LED_SAFE, OUTPUT);
  pinMode(A0, INPUT);

}



void loop() {

  int sensorState = digitalRead(sensor);
  delay(1);        // delay in between reads for stability

  if (sensorState == 1) {
    //alguem entrou :. led_alert acende e só para quando senha == senhaFixa
    entry = true;
    contando = true;
    comecarContagemAlarme();
  } else {
    tempo = 0;
    ultimoTempo = 0;
    digitalWrite(LED_ALERT, LOW);

  }
  if (entry) {
    //&& alarmeAtivado == true
    verifyEntry();

  } else {
    digitalWrite(LED_SAFE, HIGH);
    digitalWrite(LED_ALERT, LOW);
  }



  if (contando) {
    long now = millis();
    if (now - ultimoTempo >= 5000) {
      ultimoTempo = now;
      tempo++;
    }


    if (tempo > 5) {
      digitalWrite(LED_ALERT, HIGH);
      digitalWrite(LED_SAFE, LOW);
    }
  }

}

unsigned long tempoDoUltimoAvisoAlarme = 0;

void enviarSMS() {
  Serial.println(parametros);
  client.post("/sms", CONTENT_TYPE, parametros);

  int statusCode = client.responseStatusCode();
  Serial.print(F("Status da resposta: "));
  Serial.println(statusCode);

  String response = client.responseBody();
  Serial.print(F("Resposta do servidor: "));
  Serial.println(response);
}

void comecarContagemAlarme() {
  tempoDoUltimoAvisoAlarme = millis();

}
void tocarAlarme() {
  //Serial.println(F("Alarme"));
  digitalWrite(LED_ALERT, HIGH);
}
void executarAlarme() {
  if (millis() - tempoDoUltimoAvisoAlarme > 3000) {
    //Serial.println(F("Alarme"));
    tempoDoUltimoAvisoAlarme = millis();
  }
}

void casaSegura() {
  digitalWrite(LED_SAFE, HIGH);
  digitalWrite(LED_ALERT, LOW);
  Serial.println("Senha digitada: " + senha);
  Serial.println(F("Autorizado(a). Alarme desativado"));
  delay(2000);
}

void verifyEntry () {
  executarAlarme();
  char key = keypad.getKey();
  if (key) {

    if (digitandoSenha && key != '#') {
      senha += key;
      Serial.println(senha);
    }
    if (key == '*') {
      contando = false;
      tempo = 0;
      ultimoTempo = 0;
      digitalWrite(LED_ALERT, LOW);
      senha = "";
      digitandoSenha = true;
      delay(100);
      Serial.println(F("Digitando senha"));
    }
    else if (key == '#') {
      digitandoSenha = false;
      Serial.println(F("Finalizou a senha"));
      tempo = 0;
      ultimoTempo = 0;

      if (senha == "1577") {
        casaSegura();
      } else {
        digitalWrite(LED_SAFE, LOW);
        digitalWrite(LED_ALERT, HIGH);
        Serial.println("Senha digitada: " + senha);
        Serial.println(F("Não autorizado(a)"));
        delay(2000);
        enviarSMS();

      }
    }
  }
}

