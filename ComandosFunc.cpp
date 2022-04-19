#include <iostream>
#include <cstring>
#include <cstdlib>

 int respostaComando(const char* entrada){
    unsigned int n = 0;
    if(strstr(entrada, "VENTILA*")){
      return (3);
    }
   if(strstr(entrada, "EXAUST*")){
      return (4);
    }
    if(strstr(entrada, "PARAR*")){
      return (5);
    }
    if(strstr(entrada, "RETVEL*")){
      return (6);
    }  


    char* inicioDoComando = strstr(entrada, "VREL");
    if(inicioDoComando){
      
      return (2000 );
    }

    char* inicioDoComando = strstr(entrada, "VEL");
    if(inicioDoComando){
      return (1000 );
    }
      
      return (0);
  }

int main() {
    std::cout << "Hello, world!";
    const char* entrada = "BLABVRELBLA";
  
    std::cout<<respostaComando(entrada);
    return 0;

 
}