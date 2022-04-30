

//codes
// 1 - VEL
// 2 - VREL
// 3 - VENTILA
// 4 - EXAUST
// 5 - PARAR
// 6 - RETVEL
// 7 - ERRO: COMANDO INEXISTENTE
// 8 - ERRO: PARAMETRO INCORRETO
// 9 - ERRO: PARAMETRO AUSENTE

//Operacao
// 1 - soma
// 2 - subtracao

//valor - velocidade do motor
char buff[10];
int i = 0;
void parserComando(String entrada, int* operacao,int* codigo,int* valor)
{
  	*codigo = 7;
   	i = 0;
	// limpo o buffer para utilizar posteriormente 
   	for(i = 0;i<10;i++)
    {
      buff[i] = '\0';
    }
   	i = 0;
	// verifico se a entrada é igual ao codigo
    if(entrada.equals("VENTILA*")){
	  *codigo = 3;
    
    }
	// verifico se a entrada é igual ao codigo
   if(entrada.equals("EXAUST*")){
      *codigo = 4;
	
    }
	// verifico se a entrada é igual ao codigo
    if(entrada.equals("PARAR*")){
      *codigo = 5;
	
    }
	// verifico se a entrada é igual ao codigo
    if(entrada.equals("RETVEL*")){
      *codigo = 6;
	 
    }  
{	// a substring caso o comando VREL exista
    String inicioDoComando = entrada.substring(0,4);
    if(inicioDoComando.equals("VREL")&& entrada.endsWith("*"))
    {
		*codigo = 2;
		// transforma o objeto STRING para um ponteiro de char e intera 4 posicoes
		char* pstring = (char*)entrada.c_str() + 4; 
		// verifica o comando veio sem parametros
		if(*pstring == '*' || (!isDigit(*pstring) && *pstring != ' '))
		{
			*codigo = 8;
			
		}
		// verifica qual a operacao
		switch(*(++pstring))
		{
		case '+':
			*operacao = 1;
			break;
		case '-':
			*operacao = 2;
			break;
		default:
			*codigo = 8;
		}
		
      	pstring++;
		// verifica se esta sem parametros ou no formato incorreto
      	if(*pstring == '*')
        {
        	*codigo = 8;

				
        }
        if(*pstring != ' ' )
        {
        	*codigo = 7;
        }
        
		//verifica se foi passado alguma letra como parametro e alimenta o buffer
		
		while(*(++pstring) != '*')
		{
			if (!isDigit(*pstring))
			{
				*codigo = 8;
							
			}
          	if(isDigit(*pstring))
            {
            	buff[i] = *pstring;
                i++;
            }
		}
		// converte o buffer para um inteiro e alimenta o ponteiro de inteiro
      	String valorF = buff;
		*valor = valorF.toInt();

		
		//verifica se o valor esta no intervalo aceitavel
		if(*valor<0 || *valor>100)
		{
			*codigo = 8;
			
		}
			
	}
    }
	// verifica substring pelo comando VEL
    String inicioDoComando = entrada.substring(0,3);
    if(inicioDoComando.equals("VEL") && entrada.endsWith("*"))
   	{
		*codigo = 1;
		//converte objeto string em ponteiro de char e intera 3 posicoes
		char* pstring = (char*)entrada.c_str() + 3;
		// verifica se foi passado parametro
		if(*pstring == '*' || *pstring != ' ')
		{
			*codigo = 8;
			
		}
		// verifica se foi passado numeros como parametro e alimenta o buffer
		while(*(++pstring) != '*')
		{
			if (!isDigit(*pstring))
			{
				*codigo = 7;
							
			}			
          	if(isDigit(*pstring))
            {
            	buff[i] = *pstring;
                i++;
            }
		}
		// converte o buffer para inteiro e alimenta o ponteiro de inteiro
		String valorJ = buff;
		*valor = valorJ.toInt();
      	
		if(*valor<0 || *valor>100)
		{
			*codigo = 8;
			
		}
			
    }
		// verifica qual codigo foi alimentado na variavel e devolve a resposta equivalente
		switch(*codigo)
		{
			case 1:
				
				Serial.print("OK:VEL =");
				
				Serial.print(*valor);
				
				Serial.println('%');
				
				break;
			
			case 2:
				
				Serial.print("OK:VREL = ");
				
				if(*operacao == 1)
				{
					Serial.print('+');
				}
				else
				{
					Serial.print('-'); 
				}
				
				Serial.print(*valor);
				
				Serial.println('%');
				
				break;
			case 3:
				
				Serial.println("OK:VENTILA");
				
				break;
			
			case 4:
				
				Serial.println("OK:EXAUST");
				
				break;
			
			case 5:
				
				Serial.println("OK:PARAR");
				
				break;
			
			case 6:
				
				Serial.println("OK:VEL = n RPM");
				
				break;
			
			case 7:
				
				Serial.println("ERRO: COMANDO INEXISTENTE");
				
				break;
			
			case 8:
				
				Serial.println("ERRO: PARAMETRO INCORRETO");
				
				break;
			
			case 9:
				
				Serial.println("ERRO: PARAMETRO AUSENTE");
				
				break;		
		}
		
      	Serial.flush();
}
// lê a entrada uart e joga em um buffer	
String recebeComando()
{	String buffer;
	buffer=Serial.readString();
  	return buffer;
}
int incomingBytes = 10;
void setup()
{
	Serial.begin(9600);
}
int valor;
int operacao;
int codigo;

void serialEvent() {
  		String comand;
  
		comand = recebeComando();

        parserComando(comand,&operacao,&codigo,&valor);
}

void loop()
{
}
