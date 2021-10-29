#include <Keypad.h>
#include <BleKeyboard.h>
#include <AiEsp32RotaryEncoder.h>

int macroPage = 01;
// ------------------------ KEYPAD ----------------------
const int KEY_LINHAS = 4;
const int KEY_COLUNAS = 4;

char keys[KEY_LINHAS][KEY_COLUNAS] = {
    {'a', 'b', 'c', 'd'},
    {'e', 'f', 'g', 'h'},
    {'i', 'j', 'k', 'l'},
    {'*', '0', '#', 'D'},
};
byte pin_linhas[KEY_LINHAS] = {23, 19, 18, 5};    //GPIOs usados para linhas
byte pin_colunas[KEY_COLUNAS] = {22, 21, 16, 33}; //GPIOs usados para linhas

Keypad teclado = Keypad(makeKeymap(keys), pin_linhas, pin_colunas, KEY_LINHAS, KEY_COLUNAS);

const uint8_t keyConvert[16] = {KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F17, KEY_F18, KEY_F19, KEY_F20, KEY_F21,
KEY_F22, KEY_F23, KEY_F24};
//-------------------------- BLE_KEYBOARD ----------------------
BleKeyboard bleKeyboard("StreamMaster Keyboard", "BlueBox Inc.", 69);

// -------------------------- ENCODER -------------------------
#define ROTARY_ENCODER_A_PIN 15      //CLK
#define ROTARY_ENCODER_B_PIN 14      //DT
#define ROTARY_ENCODER_BUTTON_PIN 12 //SW
#define ROTARY_ENCODER_VCC_PIN -1
#define ROTARY_ENCODER_STEPS 2

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);
int lastPosition = 1000;

// --------------------------- POTENCIOMETRO -------------------
const int potPin = 13;

// ############################## Funções ######################################
void rotary_onButtonClick()
{
  static unsigned long lastTimePressed = 0;
  if (millis() - lastTimePressed < 100)
  {
    return;
  }
  lastTimePressed = millis();
  //remove on release
  Serial.println("Encoder button pressed");
  //linha que deve ser alterada para mudar a função do botão do encoder
  bleKeyboard.write(KEY_RETURN);
}

// ################################### Setup #################################################

void setup()
{
  //-----------------------------SERIAL----------------------------------------------------
  Serial.begin(115200);
  delay(1000);
  //-----------------------------ENCODER---------------------------------------------------
  rotaryEncoder.begin();
  rotaryEncoder.setup([]
                      { rotaryEncoder.readEncoder_ISR(); },
                      []
                      { rotary_onButtonClick(); });
  bool circleValues = true;
  rotaryEncoder.setBoundaries(0, 2000, circleValues);
  rotaryEncoder.disableAcceleration();
  //rotaryEncoder.setAcceleration(50);
  //-----------------------------POTENCIOMETRO----------------------------------------------

  //-----------------------------BLE_KEYBOARD-----------------------------------------------
  bleKeyboard.begin();
  Serial.println("---Bluetooth Start---");
  delay(1000);
}

// #################### Loop ####################
void loop()
{
  //---ENCODER---
  if (rotaryEncoder.encoderChanged() && bleKeyboard.isConnected())
  {
    int newPosition = rotaryEncoder.readEncoder();
    Serial.println(newPosition);
    if (newPosition > lastPosition)
    {
      bleKeyboard.write(KEY_LEFT_ARROW);
    }
    else if (newPosition < lastPosition)
    {
      bleKeyboard.write(KEY_RIGHT_ARROW);
    }
    lastPosition = newPosition;
  }
  //---POTENCIOMETRO

  //---KEYPAD---
  char botao = teclado.getKey();
  if (bleKeyboard.isConnected() && botao)
  {
    if (botao == '*')
    {
      if (macroPage == 01)
      {
        macroPage = 03;
      }
      else
      {
        macroPage--;
      };
    }
    else if (botao == '#')
    {
      if (macroPage == 03)
      {
        macroPage = 01;
      }
      else
      {
        macroPage++;
      }
    }
    else if (botao == 'D')
    {
      bleKeyboard.press(KEY_LEFT_GUI);
      bleKeyboard.press('r');
    }
    else if (botao == '0')
    {
      bleKeyboard.press(KEY_MEDIA_MUTE);
    }
    /*  
        é importante explciar porque existem duas "formatações" para as teclas, * 0 # D, e as demais teclas
        isso foi feito para facilitar a implementação e reduzir o número de "IFs",
        existe um vetor chamado keyConvert[], que contem os botões que serão enviados para o computador
        dessa forma basta usar o valor ASCII de cada tecla do keypad, para se referir a posição correta
        no vetor, dessa forma o unico if que utlizamos é para identificar qual página do teclado estamos usando
        e dessa forma inserir CTRL, SHIFT e ALT apropriadamente, é possivel utilizar várias combinações desses
        modificadores, entretanto foi utilizado apenas um modificador por página, devido ao BUG do obs que não
        diferencia entre multiplos modificadores, para obs (limite de 3 págs), sem obs (limite de 8 págs),
        utilizando um servidor python para receber serial, é possivel um número infinito de páginas e botões
        dependendo apenas da capacidae do utilizador em saber em que página de atalhos ele está.    
    */ 
    else if (macroPage == 01)
    {
      bleKeyboard.press(KEY_LEFT_CTRL);
      bleKeyboard.write(keyConvert[int(botao)-97]);
      bleKeyboard.releaseAll();
    }
    else if (macroPage == 02)
    {
      bleKeyboard.press(KEY_LEFT_SHIFT);
      bleKeyboard.write(keyConvert[int(botao)-97]);
      bleKeyboard.releaseAll();
    }
    else if (macroPage == 03)
    {
      bleKeyboard.press(KEY_LEFT_ALT);
      bleKeyboard.write(keyConvert[int(botao-97)]);
      bleKeyboard.releaseAll();
    }
    delay(100);
    //release all é lançado mais uma vez para garantir que nehum tecla estaja sendo segurada.
    bleKeyboard.releaseAll();
    //o print no serial deve ser removido na versão comercial.
    Serial.print("Tecla do keypad :");
    Serial.println(keyConvert[int(botao)-97]);
    Serial.print("Pagina de Macros: ");
    Serial.println(macroPage);
    Serial.println("---------------------------------------");
  }
}