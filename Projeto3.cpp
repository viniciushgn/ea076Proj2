#include <Wire.h >  // Biblioteca responsável por fazer a comunicação entre as portas I2C

#include <LiquidCrystal.h> //display 16x2


//recuperar os dois primeiros bytes (numero de registradores usados) a cada inicializacao 
bool medidasRecuperadas = 0;
//AVISO: DESCONECTAR E CONECTAR O ARDUINO IRA DESSINCRONIZAR O MONITOR SERIAL, É PRECISO FECHAR E ABRIR CASO ISSO OCORRA

// Definição de variáveis display 16x2:
LiquidCrystal lcd(12, 13, 7, 6, 5, 4); // Pinagem do LCD

//variaveis medir temperatura 
bool horaDeMedirTemperatura = 0;
int bufferTemperaturas[10]; //grava temperatura a cada 0.8s (soma de RA = 8, media de 10 medicoes a cada 8s)
int iteradorBuffer = 0;
float temperaturaTemporaria = 0;
bool coletaIniciada = 0; //quando em 0 ignoramos a interrupcao temporal de 0.8s

//variaveis 7 segmentos
unsigned char numero_display; // Variável criada para definir o número que deverá ser mostrado no display
int valor_display = 0;
int somaMostrada = 0; //resultado da media (de 10 temperaturas) mostrada no 7segmentos

//variaveis EEPROM
int enderecoEscrita = 1; //endereco para escrever novos dados, 16bytes little endian (menos significativo no endereco A, mais significativo no endereco A+1), endereco 0 REZERVADO PARA O NUMERO DE REGISTROS USADOS

// Definição de variáveis funcoes/display
String ultimosDois; //ultimos dois caracteres apertados no teclado pelo usuario

// Definição de variáveis teclado:
String teclasAntigas; // Variável String criada para armazenar o valor das teclas que foram apertadas
String teclasAtuais; // Variável String criada para armazenar o valor das teclas que estão sendo apertadas

// Configuração do Programa:>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void setup() {
   Wire.begin(); //Inicia a biblioteca Wire

   Serial.begin(9600); //Inicia a comunicação serial 
   //INTERRUPCOES----------------------------------------------------------------------------------------- 
  
   //Queremos armazenar de 8 em 8s a média de 10 medições de temperatura:
   //= 1/8 * 0.1 Hz

   // TIMER 1 PARA interrupt frequency 1.25 Hz:
   cli(); // parar interrrupcoes
   TCCR1A = 0; // set registrador TCCR1A para 0
   TCCR1B = 0; //mesmo para TCCR1B
   TCNT1 = 0; // inicializar counter value em 0
   // set registrador compare match  para incrementos em 1.25 Hz 
   OCR1A = 15000; // = 16000000 / (256 * 1.25) - 1 (deve ser < 65536)
   // ligando CTC mode
   TCCR1B |= (1 << WGM12);
   // Set bits CS12, CS11 e CS10 para prescaler de 256
   TCCR1B |= (1 << CS12) | (0 << CS11) | (0 << CS10);
   // enable timer compare interrupt
   TIMSK1 |= (1 << OCIE1A);
   sei(); // permitir interrupcoes

   //------------------------------------------------------------------------------------------------------
   // TECLADO----------------------------------------------------------------------------------------------
   //setando as entradas (4 linhas) que vão receber TENSÃO HIGH em todas as linhas menos uma, alternada uma
   //de cada vez. 1000,0100,0010,0001.

   pinMode(A1, OUTPUT); // Define L1 como saída
   pinMode(A2, OUTPUT); // Define L2 como saída
   pinMode(A3, OUTPUT); // Define L3 como saída
   pinMode(2, OUTPUT); // Define L4 como saída

   //setando as saidas (3 colunas) por onde vamos LER se há ou não tensão e, em conjunto com a informação de
   //qual LINHA está "ATERRADA", descobrir qual tecla está pressionada
   pinMode(9, INPUT_PULLUP); // Define C1 como saída
   pinMode(8, INPUT_PULLUP); // Define C2 como saída
   pinMode(3, INPUT_PULLUP); // Define C3 como saída
   //----------------------------------------------------------------------------------------------------
   //LER TEMPERATURA----------------------------------------------------------------------------------------------
   pinMode(A0, INPUT); //lemos o OUT do sensor de temperatura no A0

   //------------------------------------------------------------------------

   //LCD---------------------------------------
   lcd.begin(16, 2); // Inicia o lcd de 16x2
   //------------------------------------------
analogReference(2); //mais detalhamento na tensao medida 1.1v
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float lerTemperatura() {
   return (analogRead(A0) * 500 / 1023);
   //*
}

//EEPROM___
//FUNCAO WRITE1: recebe endereco (max 2047), recebe dado (max 255), atualiza por i2c os valores na memoria EEPROM
void write1_EEPROM(unsigned int endereco, unsigned int dado) {
   //Vamos mandar 1 bytes para a EEPROM!
   //primeiramente, vamos mandar o device adress (1010) com os 3 bits mais significativos do endereco da pagina (onde queremos escrever)
   //podemos fingir que todos os bits de enderecamento (página e endereco) são um único bloco 
   int enderecoBegin = 0x50 | ((endereco >> 8) & 7);
   Wire.beginTransmission(enderecoBegin); //truncamos o device address (com um OR) com os 3 bits mais significativos do endereco para write

   Wire.write(endereco & 255); //mandamos os outros 4 bits que faltavam do endereco (P3 P2 P1 P0 D3 D2 D1 D0)

   Wire.write(dado); // Escreve o dado
   Wire.endTransmission(); // Manda o sinal de parada (escada de descida)

}

//FUNCAO WRITE2: recebe endereco (max 2047), recebe dado1 (max 255), recebe dado2 (max 255), escreve por i2c o dado1 no endereco selecionado e o dado2 no endereco selecionado somado de 1
void write2_EEPROM(unsigned int endereco, unsigned int dado1, unsigned int dado2) {
   //Vamos mandar 2 bytes (1 medida) para a EEPROM!
   //primeiramente, vamos mandar o device adress (1010) com os 3 bits mais significativos do endereco da pagina (onde queremos escrever)
   //podemos fingir que todos os bits de enderecamento (página e endereco) são um único bloco 
   int enderecoBegin = 0x50 | ((endereco >> 8) & 7);
   Wire.beginTransmission(enderecoBegin); //truncamos o device address (com um OR) com os 3 bits mais significativos do endereco para write

   Wire.write(endereco & 255); //mandamos os outros 4 bits que faltavam do endereco (P3 P2 P1 P0 D3 D2 D1 D0)

   Wire.write(dado1); // Escreve o dado1
   Wire.write(dado2); // Escreveo dado2
   Wire.endTransmission(); // Manda o sinal de parada (escada de descida)

}

//FUNCAO LERMEDIDA: retorna a medida a ser lida a partir do enderecoInicial especificado
void EEPROM_readMedida(int endereco) {
   int enderecoBegin = 0x50 + ((0xFF & endereco) >> 8);
   int valor;
   int valor2;
   int saida;
   Wire.beginTransmission(enderecoBegin);

   unsigned int enderecoResto = (0xFF & endereco);
   Wire.write(enderecoResto);
   Wire.endTransmission();
   delay(5);

   Wire.requestFrom(0x50, 2);
   valor = Wire.read();
   valor2 = Wire.read();

   saida = (int)(((unsigned) valor2 << 8) | valor);

   Serial.println(saida);

}

int EEPROM_readRegistradores(int endereco) {
   int address = 0x50 + ((0b11100000000 & endereco) >> 8);
   int valor;
   int valor2;
   int saida;
   Wire.beginTransmission(address);

   unsigned int endData = (0b11111111 & endereco);
   Wire.write(endData);
   Wire.endTransmission();
   delay(5);

   Wire.requestFrom(0x50, 2);
   valor = Wire.read();
   valor2 = Wire.read();

   saida = (int)(((unsigned) valor2 << 8) | valor);

   return saida;

}
//___

// Função criada botoesApertados - retorna o valor da String teclas contendo as teclas que foram apertadas pelo
//usuário
// DEPENDENCIA: Setup dos pinos das Colunas (L1-A1,L2-A2,L3-A3, L4-2) e Linhas (C1-9,C2-8,C3-3)
String botoesApertados() {
   int val = 0; // variável inteira para guardar o valor lido
   String teclas; // Variável String para mostrar qual tecla foi apertada
   // L1 ligado <<
   digitalWrite(A1, LOW); // Coloca L1 em LOW
   digitalWrite(A2, HIGH); // Coloca L2 em HIGH
   digitalWrite(A3, HIGH); // Coloca L3 em HIGH
   digitalWrite(2, HIGH); // Coloca L4 em HIGH
   delay(1); // Delay criado para evitar deboucing
   // Lendo e armazenando tecla
   val = digitalRead(9); // Lê a coluna C1
   if (val == 0) {
      teclas = teclas + "-" + "1"; // Se C1 estiver em LOW, concatena "-1" na variável String teclas
   }

   val = digitalRead(8); // Lê a coluna C2
   if (val == 0) {
      teclas = teclas + "-" + "2"; // Se C2 estiver em LOW, concatena "-2" na variável String teclas
   }

   val = digitalRead(3); // Lê a coluna C3
   if (val == 0) {
      teclas = teclas + "-" + "3"; // Se C3 estiver em LOW, concatena "-3" na variável String teclas
   }

   // L2 ligado <<
   digitalWrite(A1, HIGH); // Coloca L1 em HIGH
   digitalWrite(A2, LOW); // Coloca L2 em LOW
   digitalWrite(A3, HIGH); // Coloca L3 em HIGH
   digitalWrite(2, HIGH); // Coloca L4 em HIGH
   delay(1); // Delay criado para evitar deboucing

   // Lendo e armazenando tecla
   val = digitalRead(9); // Lê a coluna C1
   if (val == 0) {
      teclas = teclas + "-" + "4"; // Se C1 estiver em LOW, concatena "-4" na variável String teclas
   }

   val = digitalRead(8); // Lê a coluna C2
   if (val == 0) {
      teclas = teclas + "-" + "5"; // Se C2 estiver em LOW, concatena "-5" na variável String teclas
   }

   val = digitalRead(3); // Lê a coluna C3
   if (val == 0) {
      teclas = teclas + "-" + "6"; // Se C3 estiver em LOW, concatena "-6" na variável String teclas
   }

   // L3 ligado <<
   digitalWrite(A1, HIGH); //Coloca L1 em HIGH
   digitalWrite(A2, HIGH); //Coloca L2 em HIGH
   digitalWrite(A3, LOW); //Coloca L3 em LOW
   digitalWrite(2, HIGH); //Coloca L4 em HIGH
   delay(1); // Delay criado para evitar deboucing

   //lendo e armazenando tecla
   val = digitalRead(9); // Lê a coluna C1
   if (val == 0) {
      teclas = teclas + "-" + "7"; // Se C1 estiver em LOW, concatena "-7" na variável String teclas
   }

   val = digitalRead(8); // Lê a coluna C2
   if (val == 0) {
      teclas = teclas + "-" + "8"; // Se C2 estiver em LOW, concatena "-8" na variável String teclas
   }

   val = digitalRead(3); // Lê a coluna C3
   if (val == 0) {
      teclas = teclas + "-" + "9"; // Se C3 estiver em LOW, concatena "-9" na variável String teclas
   }

   // L4 ligado <<
   digitalWrite(A1, HIGH); //Coloca L1 em HIGH
   digitalWrite(A2, HIGH); //Coloca L2 em HIGH
   digitalWrite(A3, HIGH); //Coloca L3 em HIGH
   digitalWrite(2, LOW); //Coloca L4 em LOW
   delay(1); // Delay criado para evitar deboucing

   //Lendo e armazenando tecla
   val = digitalRead(3); // Lê a coluna C3

   if (val == 0) {
      teclas = teclas + "-" + ""; // Se C1 estiver em LOW, concatena "-" na variável String teclas
      ultimosDois = "-*";
   }

   val = digitalRead(8); // Lê a coluna C2
   if (val == 0) {
      teclas = teclas + "-" + "0"; // Se C2 estiver em LOW, concatena "-0" na variável String teclas
   }

   val = digitalRead(9); // Lê a coluna C1
   if (val == 0) {
      teclas = teclas + "-" + "#"; // Se C3 estiver em LOW, concatena "-#" na variável String teclas
   }

   return teclas; // A função retorna o valor da String teclas, onde está o valor das teclas que foram apertadas pelo
   //usuário
}

// Função criada lidaTeclas - Retorna sempre o valor das teclas que foram apertadas atualmente
String lidaTeclas(String teclasAtuais, String teclasAntigas) {
   if (teclasAtuais == teclasAntigas) {
      return "";
   } else {
      if (teclasAtuais != "-*") {
         ultimosDois = ultimosDois + teclasAtuais;
         if (ultimosDois.length() > 4) {
            //se maior que 4 (precisamos de dois valores para interpretar funcao, numero e o comando para executar
            ultimosDois.remove(0, 2); //tiramos os dois primeiros caracteres para abrir espaco aos novos
         }
      }

      return teclasAtuais;
   }
}

//funcoes de cada caso -------------------------------------------------------------------------------------------------------------------------
void funcaoCaso1() {
   String memoriaDeletada;
   memoriaDeletada = enderecoEscrita - 1;
   lcd.clear();
   lcd.setCursor(0, 0); // 0 = 0 colunas para a direita. 0 = Primeira linha
   lcd.print(memoriaDeletada + " MEDIDAS"); // Imprime no LCD
   // lcd.setCursor(12,0); // 12 = 12 colinas para a direita. 0 = Primeira linha
   lcd.setCursor(0, 1); // 0 colunas para a direita. 1 = Segunda linha
   lcd.print("DELETADAS");
   enderecoEscrita = 1; //zeramos nosso endereco de escrita de dados na EEPROM, novos dados serao escritos em cima dos dados antigos 
   delay(5);
   write2_EEPROM(0, (unsigned) 1 & 0xff, (unsigned) 1 >> 8); //zeramos nossos 2 primeiros bytes (1 endereco usado = primeiros 2 registradores usados para salvar quantas medidas estao na EEPROM)
   delay(3000);
   ultimosDois = "-*";
}

void funcaoCaso2() {
   String memoriaUsada;
   memoriaUsada = enderecoEscrita - 1;
   lcd.clear();
   lcd.setCursor(0, 0); // 0 = 0 colunas para a direita. 0 = Primeira linha
   lcd.print(memoriaUsada + " MEDIDAS"); // Imprime a rotação estimada no LCD
   // lcd.setCursor(12,0); // 12 = 12 colinas para a direita. 0 = Primeira linha
   lcd.setCursor(0, 1); // 3 = 3 colunas para a direita. 1 = Segunda linha
   memoriaUsada = 1024 - enderecoEscrita;
   lcd.print("RESTA " + memoriaUsada);
   delay(3000);
   ultimosDois = "-*";
}

void funcaoCaso3() {
   lcd.clear();
   lcd.setCursor(0, 0);
   lcd.print("COLETA");
   lcd.setCursor(0, 1);
   lcd.print("INICIADA");
   coletaIniciada = 1; //habilitamos a funcao que lida com coletas a aceitar as interrupcoes temporais
   delay(3000);
   ultimosDois = "-*";
}

void funcaoCaso4() {
   lcd.clear();
   lcd.setCursor(0, 0);
   lcd.print("COLETA");
   lcd.setCursor(0, 1);
   lcd.print("FINALIZADA");
   coletaIniciada = 0; //desabilitamos a funcao de coleta
   delay(3000);
   ultimosDois = "-*";
}

void funcaoCaso5() {
   bool cancelaFunc = 0;
   String teclaAtual;
   String s_tamanhoN;
   unsigned int tamanhoN = 0;

   lcd.setCursor(0, 0);
   lcd.print("Quantas medidas?");
   // lcd.setCursor(12,0); 
   lcd.setCursor(0, 1);
   lcd.print("N=");
   while (teclaAtual != "-#" && !cancelaFunc) //enquanto nao confirmamos nem cancelamos o numero de medidas a ser transmitido, ficamos esperando numeros novos do teclado
   {
      lcd.clear();

      teclaAtual.remove(0, 1);
      if (teclaAtual != "*") {
         s_tamanhoN = s_tamanhoN + teclaAtual;
      } else {
         cancelaFunc = 1;
      }

      lcd.setCursor(0, 0); // 0 = 0 colunas para a direita. 0 = Primeira linha
      lcd.print("Quantas medidas?"); // Imprime a rotação estimada no LCD
      // lcd.setCursor(12,0); // 12 = 12 colinas para a direita. 0 = Primeira linha
      lcd.setCursor(0, 1); // 3 = 3 colunas para a direita. 1 = Segunda linha
      lcd.print("N=" + s_tamanhoN); // Imprime "ESTIMATIVA" centralizado na segunda linha 

      teclasAtuais = botoesApertados(); // Armazena o valor das teclas que estão sendo apertadas
      teclaAtual = lidaTeclas(teclasAtuais, teclasAntigas);
      teclasAntigas = teclasAtuais; // As teclas apertadas atualmente sao guardadas
      delay(60); //refresh rate do display mais agradavel
   }

   ultimosDois = "-*";
   if (!cancelaFunc) {
      //se nao cancelamos a funcao apertando o asterisco rodamos o fim da funcao
      tamanhoN = s_tamanhoN.toInt();

      if (tamanhoN > enderecoEscrita - 1) {
         s_tamanhoN = enderecoEscrita;
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("ERRO: LIMITE DE");
         // lcd.setCursor(12,0); 
         lcd.setCursor(0, 1);
         lcd.print(s_tamanhoN + " medidas");
         delay(3000);
      } else {
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("Transferindo as");
         // lcd.setCursor(12,0); 
         lcd.setCursor(0, 1);
         lcd.print(s_tamanhoN + " medidas");

         //lendo medidas
         for (int i = 0; i < tamanhoN; i++) {

            EEPROM_readMedida(enderecoEscrita * 2 - 2 - 2 * i); //lemos de dois em dois bytes (limite de 1 byte 256, precisamos de dois little endian) movimentando os bits de forma a reconstruir o int original

         }

         delay(3000);
      }
   }
}

//FIM funcoes de cada caso -------------------------------------------------------------------------------------------------------------------------

//funcao para lidar com as funcoes digitadas pelo usuario

void interpretaFuncao(String comando) {
   lcd.clear();
   delay(1);
   //CASO 1
   if (comando.indexOf('1') != -1) //achamos um "1" no comando
   {
      if (comando != "-1-#") //se nao esta sendo executado
      {
         if (comando.lastIndexOf('1') == 3) //vemos se o 1 eh o ultimo numero digitado (mais relevante ao usuario)
         {
            lcd.setCursor(0, 0);
            lcd.print("FUNCAO: RESET");
            // lcd.setCursor(12,0);   
            lcd.setCursor(0, 1);
            lcd.print("STATUS: SELECTED"); //mostramos que a funcao 1 foi selecionada
            return;
         }
      } else {
         lcd.setCursor(0, 0);
         lcd.print("FUNCAO: RESET");
         // lcd.setCursor(12,0);
         lcd.setCursor(0, 1);
         lcd.print("STATUS: EXECUTAR"); // caso estiver sendo executada (string -1-# = ultimos botoes apertados) rodamos a funcao 1
         //EXECUTA FUNCAO AQUI
         funcaoCaso1();
         return;
      }
   }
   //LOGICA EQUIVALENTE APLICADA EM TODOS OS CASOS 1-5
   //CASO 2
   if (comando.indexOf('2') != -1) {
      if (comando != "-2-#") {
         if (comando.lastIndexOf('2') == 3) {
            lcd.setCursor(0, 0);
            lcd.print("FUNCAO: Ocupacao");
            // lcd.setCursor(12,0); 
            lcd.setCursor(0, 1);
            lcd.print("STATUS: SELECTED");
            return;
         }
      } else {
         lcd.setCursor(0, 0);
         lcd.print("FUNCAO: Ocupacao");
         // lcd.setCursor(12,0); 
         lcd.setCursor(0, 1);
         lcd.print("STATUS: EXECUTAR");
         //EXECUTA FUNCAO AQUI
         funcaoCaso2();
         return;
      }
   }

   //CASO 3
   if (comando.indexOf('3') != -1) {
      if (comando != "-3-#") {
         if (comando.lastIndexOf('3') == 3) {
            lcd.setCursor(0, 0);
            lcd.print("FUNCAO:INIColeta");
            // lcd.setCursor(12,0); 
            lcd.setCursor(0, 1);
            lcd.print("STATUS: SELECTED");
            return;
         }
      } else {
         lcd.setCursor(0, 0);
         lcd.print("FUNCAO:INIColeta");
         // lcd.setCursor(12,0);
         lcd.setCursor(0, 1);
         lcd.print("STATUS: EXECUTAR");
         //EXECUTA FUNCAO AQUI
         funcaoCaso3();
         return;
      }
   }

   //CASO 4
   if (comando.indexOf('4') != -1) {
      if (comando != "-4-#") {
         if (comando.lastIndexOf('4') == 3) {
            lcd.setCursor(0, 0);
            lcd.print("FUNCAO:FINColeta");
            // lcd.setCursor(12,0); 
            lcd.setCursor(0, 1);
            lcd.print("STATUS: SELECTED");
            return;
         }
      } else {
         lcd.setCursor(0, 0);
         lcd.print("FUNCAO:FINColeta");
         // lcd.setCursor(12,0);
         lcd.setCursor(0, 1);
         lcd.print("STATUS: EXECUTAR");
         //EXECUTA FUNCAO AQUI
         funcaoCaso4();
         return;
      }
   }

   //CASO 5
   if (comando.indexOf('5') != -1) {
      if (comando != "-5-#") {
         if (comando.lastIndexOf('5') == 3) {
            lcd.setCursor(0, 0);
            lcd.print("FUNCAO:TransfDad");
            // lcd.setCursor(12,0);
            lcd.setCursor(0, 1);
            lcd.print("STATUS: SELECTED");
            return;
         }
      } else {
         lcd.setCursor(0, 0);
         lcd.print("FUNCAO:TransfDad");
         // lcd.setCursor(12,0); 
         lcd.setCursor(0, 1);
         lcd.print("STATUS: EXECUTAR");
         //EXECUTA FUNCAO AQUI
         funcaoCaso5();
         return;
      }
   }

   //NAO ENTROU EM NENHUM CASO
   lcd.setCursor(0, 0);
   lcd.print("FUNCAO:");
   // lcd.setCursor(12,0);
   lcd.setCursor(0, 1);
   lcd.print("STATUS: "); // FUNCAO NULA STATUS NULO
}

//INTERRUPCAO TEMPORAL__ 
ISR(TIMER1_COMPA_vect) {
   horaDeMedirTemperatura = 1; //a cada 0.8s ligamos essa "flag" booleana para permitir no loop que a funcao de lidar com temperaturas medidas faca medicoes
}

//__
//funcao mostrar no 7 segmentos
void mostra_display(int valor) {
   valor_display = (valor / 1000) % 10; // Quebra a variável nos dígitos que a compoem
   numero_display = (valor_display << 4) | 14; // Define o valor que deverá ser mostrado no display mais significativo
   Wire.beginTransmission(32); //Essa função inicia a transmissão para o endereço 32 do PCF8574
   Wire.write(numero_display); // Transmite o respectivo valor para o display mais significativo                         
   Wire.endTransmission(); // Termina a transmissão
   delay(3);
   valor_display = (valor / 100) % 10; // Quebra a variável nos dígitos que a compoem
   numero_display = (valor_display << 4) | 13; // Define o valor que deverá ser mostrado no segundo display mais significativo
   Wire.beginTransmission(32); //Essa função inicia a transmissão para o endereço 32 do PCF8574
   Wire.write(numero_display); // Transmite o respectivo valor para o segundo display mais significativo
   Wire.endTransmission(); // Termina a transmissão
   delay(2);
   valor_display = (valor / 10) % 10; // Quebra a variável nos dígitos que a compoem
   numero_display = (valor_display << 4) | 11; // Define o valor que deverá ser mostrado no terceiro display mais significativo
   Wire.beginTransmission(32); //Essa função inicia a transmissão para o endereço 32 do PCF8574
   Wire.write(numero_display); // Transmite o respectivo valor para o terceiro display mais significativo
   Wire.endTransmission(); // Termina a transmissão
   delay(1);
   valor_display = (valor) % 10; // Quebra a variável nos dígitos que a compoem
   numero_display = (valor_display << 4) | 7; // Define o valor que deverá ser mostrado no quarto display mais significativo
   Wire.beginTransmission(32); //Essa função inicia a transmissão para o endereço 32 do PCF8574
   Wire.write(numero_display); // Transmite o respectivo valor para o display menos significativo
   Wire.endTransmission(); // Termina a transmissão  
   delay(0);
}

//ARMAZENAR TEMPERATURA____
void lidarTemperatura() {
   if (coletaIniciada) //coleta foi habilitada pela funcao 3
   {
      if (horaDeMedirTemperatura == 1) //flag foi habilitada pela interupcao que eh chamada a cada 0.8s
      {
         horaDeMedirTemperatura = 0; //flag desligada
         temperaturaTemporaria = lerTemperatura(); //lemos a temperatura
         if (temperaturaTemporaria < 0) //somente aceitamos valores positivos (condicao permitida pelo escopo do projeto)
         {
            return;
         }

         bufferTemperaturas[iteradorBuffer] = temperaturaTemporaria * 10; //armazenamos o int da temperatura com 1 casa decimal 
         if (iteradorBuffer < 9) {
            iteradorBuffer++;
         } else //caso passamos de 10 MEDICOES vamos calcular a media e armazenar (de 8 em 8 segundos, RA188182 RA185846, 2+6=8)
         {
            float soma = 0;
            iteradorBuffer = 0;
            for (int i = 0; i < 9; i++) {
               soma = soma + bufferTemperaturas[i]; //soma total do buffer
            }

            soma = soma / 10; //calculamos a media
            somaMostrada = soma;
            write2_EEPROM(enderecoEscrita * 2, (unsigned) somaMostrada & 0xff, (unsigned) somaMostrada >> 8); //dividindo dado em 2 bytes little endian (matando bits superiores, movendo bits superiores pra baixo)
            enderecoEscrita++;
            delay(5); //escrita consecutiva em enderecos diferentes precisa desse delay
            write2_EEPROM(0, (unsigned) enderecoEscrita & 0xff, (unsigned) enderecoEscrita >> 8); //salvando nos 2 primeiros bytes da EEPROM nosso NUMERO DE MEMORIAS USADAS
            Serial.println(somaMostrada);

            delay(5);

            //     Serial.println(somaMostrada);   // PARA MOSTRAR NO SERIAL QUAL TEMPERATURA FOI ESCRITA NA EEPROM, FUNCAO DE DEBUG IMPORTANTE

         }
      }
   }
}

//___

//-----------------------------------------------------------------------------------------------------------
// Programa Principal em loop
void loop() {
   if (!medidasRecuperadas) {
      medidasRecuperadas = 1;
      enderecoEscrita = EEPROM_readRegistradores(0);
      Serial.println(enderecoEscrita);
   }

   teclasAtuais = botoesApertados(); // Armazena o valor das teclas que estão sendo apertadas
   lidaTeclas(teclasAtuais, teclasAntigas); //lida com debounces, repeticoes
   teclasAntigas = teclasAtuais; // As teclas armazenadas para efeito de analise do comando sao atualizadas

   mostra_display(somaMostrada); //ultima media calculada mostrada no display 7 segmentos

   interpretaFuncao(ultimosDois); //usamos os ultimos dois caracteres apertados no teclado pelo usuario para decidir qual funcao (em qual status) foi escolhida

   lidarTemperatura(); //medida executada a cada 0.8s, medida media (de 10 valores) armazenada a cada 8s (RA 2 + 6)

}
