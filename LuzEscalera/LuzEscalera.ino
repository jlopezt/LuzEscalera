/*
 * Actuador con E/S
 *
 * Actuador remoto
 * 
 * Reles de conexion y entradas digitales
 * 
 * Servicio web levantado en puerto PUERTO_WEBSERVER
 */

/***************************** Defines *****************************/
//Defines generales
#define NOMBRE_FAMILIA   "Actuador/Secuenciador (E/S)"
#define VERSION          "4.5.2 (ESP8266v2.4.2 OTA|MQTT|Logic+|Secuenciador|eventos SNTP)" //Alineado con secuenciador 4.5.2
#define SEPARADOR        '|'
#define SUBSEPARADOR     '#'
#define KO               -1
#define OK                0
#define MAX_VUELTAS      UINT16_MAX// 32767 

//definiciones de modulos
#define ENTRADAS_SALIDAS
#undef  MAQUINA_ESTADOS
#undef  SECUENCIADOR 

//Ficheros de configuracion
#define FICHERO_CANDADO                  "/Candado"
#define FICHERO_ERRORES                  "/Errores.log"
#define GLOBAL_CONFIG_FILE               "/Config.json"
#define GLOBAL_CONFIG_BAK_FILE           "/Config.json.bak"
#define ENTRADAS_CONFIG_FILE             "/EntradasConfig.json"
#define ENTRADAS_CONFIG_BAK_FILE         "/EntradasConfig.json.bak"
#define SALIDAS_CONFIG_FILE              "/SalidasConfig.json"
#define SALIDAS_CONFIG_BAK_FILE          "/SalidasConfig.json.bak"
#define WIFI_CONFIG_FILE                 "/WiFiConfig.json"
#define WIFI_CONFIG_BAK_FILE             "/WiFiConfig.json.bak"
#define MQTT_CONFIG_FILE                 "/MQTTConfig.json"
#define MQTT_CONFIG_BAK_FILE             "/MQTTConfig.json.bak"
#define SECUENCIADOR_CONFIG_FILE         "/SecuenciadorConfig.json"
#define SECUENCIADOR_CONFIG_BAK_FILE     "/SecuenciadorConfig.json.bak"
#define GHN_CONFIG_FILE                  "/GHNConfig.json"
#define GHN_CONFIG_BAK_FILE              "/GHNConfig.json.bak"
#define MAQUINAESTADOS_CONFIG_FILE       "/MaqEstadosConfig.json"
#define MAQUINAESTADOS_CONFIG_BAK_FILE   "/MaqEstadosConfig.json.bak"

// Una vuela de loop son ANCHO_INTERVALO segundos 
#define ANCHO_INTERVALO                 100 //Ancho en milisegundos de la rodaja de tiempo
#define FRECUENCIA_OTA                    5 //cada cuantas vueltas de loop atiende las acciones
#define FRECUENCIA_ENTRADAS               5 //cada cuantas vueltas de loop atiende las entradas
#define FRECUENCIA_SALIDAS                5 //cada cuantas vueltas de loop atiende las salidas
#define FRECUENCIA_SECUENCIADOR          10 //cada cuantas vueltas de loop atiende al secuenciador
#define FRECUENCIA_MAQUINAESTADOS        10 //cada cuantas vueltas de loop atiende a la maquina de estados
#define FRECUENCIA_SERVIDOR_WEB           1 //cada cuantas vueltas de loop atiende el servidor web
//#define FRECUENCIA_SERVIDOR_WEBSOCKET     1 //cada cuantas vueltas de loop atiende el servidor web
#define FRECUENCIA_MQTT                  10 //cada cuantas vueltas de loop envia y lee del broker MQTT
#define FRECUENCIA_ENVIO_DATOS          100 //cada cuantas vueltas de loop envia al broker el estado de E/S
#define FRECUENCIA_ORDENES                2 //cada cuantas vueltas de loop atiende las ordenes via serie 
#define FRECUENCIA_SNTP                 100 //cada cuantas vueltas de loop atiende los eventos SNTP recibidos
#define FRECUENCIA_WIFI_WATCHDOG        100 //cada cuantas vueltas comprueba si se ha perdido la conexion WiFi
/***************************** Defines *****************************/

/***************************** Includes *****************************/
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
//#include <WebSocketsServer.h>
/***************************** Includes *****************************/

/***************************** variables globales *****************************/
//Indica si el rele se activa con HIGH o LOW
int nivelActivo;

String nombre_dispositivo;//(NOMBRE_FAMILIA);//Nombre del dispositivo, por defecto el de la familia
uint16_t vuelta = 0;//MAX_VUELTAS-100; //vueltas de loop
int debugGlobal=0; //por defecto desabilitado
boolean candado=false; //Candado de configuracion. true implica que la ultima configuracion fue mal
/***************************** variables globales *****************************/
/************************* FUNCIONES PARA EL BUITIN LED ***************************/
void configuraLed(void){pinMode(LED_BUILTIN, OUTPUT);}
void enciendeLed(void){digitalWrite(LED_BUILTIN, LOW);}
void apagaLed(void){digitalWrite(LED_BUILTIN, HIGH);}
void parpadeaLed(uint8_t veces, uint16_t espera=100)
  {
  for(uint8_t i=0;i<2*veces;i++)
    {
    delay(espera/2);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
  }
/***********************************************************************************/  

/*************************************** SETUP ***************************************/
void setup()
  {
  Serial.begin(115200);
  configuraLed();
  enciendeLed();
    pinMode(14,OUTPUT);

  Serial.printf("\n\n\n");
  Serial.printf("*************** %s ***************\n",NOMBRE_FAMILIA);
  Serial.printf("*************** %s ***************\n",VERSION);
  Serial.println("***************************************************************");
  Serial.println("*                                                             *");
  Serial.println("*             Inicio del setup del modulo                     *");
  Serial.println("*                                                             *");    
  Serial.println("***************************************************************");

  Serial.printf("\n\nInit Ficheros ---------------------------------------------------------------------\n");
  //Ficheros - Lo primero para poder leer los demas ficheros de configuracion
  inicializaFicheros(debugGlobal);

  //Compruebo si existe candado, si existe la ultima configuracion fue mal
  if(existeFichero(FICHERO_CANDADO)) 
    {
    Serial.printf("Candado puesto. Configuracion por defecto");
    candado=true; 
    debugGlobal=1;
    }
  else
    {
    candado=false;
    //Genera candado
 /*   if(salvaFichero(FICHERO_CANDADO,"","JSD")) Serial.println("Candado creado");
    else Serial.println("ERROR - No se pudo crear el candado");*/
    }
 
  //Configuracion general
  Serial.printf("\n\nInit General ---------------------------------------------------------------------\n");
  inicializaConfiguracion(debugGlobal);
  parpadeaLed(1);

  //Wifi
  Serial.println("\n\nInit WiFi -----------------------------------------------------------------------\n");
  if (inicializaWifi(1))
    {
    parpadeaLed(5,200); 
    /*----------------Inicializaciones que necesitan red-------------*/
    //OTA
    Serial.println("\n\nInit OTA ------------------------------------------------------------------------\n");
    inicializaOTA(debugGlobal);
    parpadeaLed(1);
    //SNTP
    Serial.printf("\n\nInit SNTP ------------------------------------------------------------------------\n");
    inicializaReloj();    
    parpadeaLed(2);
    //MQTT
    Serial.println("\n\nInit MQTT -----------------------------------------------------------------------\n");
    inicializaMQTT();
    parpadeaLed(3);
    //WebServer
    Serial.println("\n\nInit Web ------------------------------------------------------------------------\n");
    inicializaWebServer();
    /*
    //WebSockets
    Serial.println("\n\nInit WebSockets -----------------------------------------------------------------\n");
    inicializaWebSockets();
    */
    //mDNS
    Serial.println("\n\nInit mDNS -----------------------------------------------------------------------\n");
    inicializamDNS(NULL);
    parpadeaLed(3);
    }
  else Serial.println("No se pudo conectar al WiFi");
  apagaLed();

  //Entradas
  Serial.println("\n\nInit entradas ------------------------------------------------------------------------\n");
  inicializaEntradas();

  //Salidas
  Serial.println("\n\nInit salidas ------------------------------------------------------------------------\n");
  inicializaSalidas();

  //Secuenciador
  Serial.println("\n\nInit secuenciador ---------------------------------------------------------------------\n");
  inicializaSecuenciador();
  
  //Maquina de estados
  Serial.println("\n\nInit maquina de estados----------------------------------------------------------------\n");
  inicializaMaquinaEstados();
  
  //Ordenes serie
  Serial.println("\n\nInit Ordenes ----------------------------------------------------------------------\n");  
  inicializaOrden();//Inicializa los buffers de recepcion de ordenes desde PC

  //Si ha llegado hasta aqui, todo ha ido bien y borro el candado
  if(borraFichero(FICHERO_CANDADO))Serial.println("Candado borrado");
  else Serial.println("ERROR - No se pudo borrar el candado");
  
  compruebaConfiguracion(0);
  parpadeaLed(2);
  apagaLed();//Por si acaso...
  
  Serial.printf("\n\n");
  Serial.println("***************************************************************");
  Serial.println("*                                                             *");
  Serial.println("*               Fin del setup del modulo                      *");
  Serial.println("*                                                             *");    
  Serial.println("***************************************************************");
  Serial.printf("\n\n");  

  //activo el watchdog
  ESP.wdtEnable(1);  
  }  

void loop()
  {  
  //referencia horaria de entrada en el bucle
  time_t EntradaBucle=0;
  EntradaBucle=millis();//Hora de entrada en la rodaja de tiempo

  //reinicio el watchdog del sistema
  ESP.wdtFeed();
  
  //------------- EJECUCION DE TAREAS --------------------------------------
  //Acciones a realizar en el bucle   
  //Prioridad 0: OTA es prioritario.
  if ((vuelta % FRECUENCIA_OTA)==0) gestionaOTA(); //Gestion de actualizacion OTA
  //Prioridad 2: Funciones de control.
  if ((vuelta % FRECUENCIA_ENTRADAS)==0) consultaEntradas(debugGlobal); //comprueba las entradas
  if ((vuelta % FRECUENCIA_SALIDAS)==0) actualizaSalidas(debugGlobal); //comprueba las salidas
  if ((vuelta % FRECUENCIA_SECUENCIADOR)==0) actualizaSecuenciador(debugGlobal); //Actualiza la salida del secuenciador
  if ((vuelta % FRECUENCIA_SNTP)==0) actualizaSNTP(debugGlobal); //Actualiza los eventos SNT recibidos
  if ((vuelta % FRECUENCIA_MAQUINAESTADOS)==0) actualizaMaquinaEstados(debugGlobal); //Actualiza la maquina de estados
  //Prioridad 3: Interfaces externos de consulta    
  if ((vuelta % FRECUENCIA_SERVIDOR_WEB)==0) webServer(debugGlobal); //atiende el servidor web
  //if ((vuelta % FRECUENCIA_SERVIDOR_WEBSOCKET)==0) atiendeWebSocket(debugGlobal); //atiende el servidor web
  if ((vuelta % FRECUENCIA_MQTT)==0) atiendeMQTT();      
  if ((vuelta % FRECUENCIA_ENVIO_DATOS)==0) enviaDatos(debugGlobal); //publica via MQTT los datos de entradas y salidas, segun configuracion
  if ((vuelta % FRECUENCIA_ORDENES)==0) while(HayOrdenes(debugGlobal)) EjecutaOrdenes(debugGlobal); //Lee ordenes via serie
  //Prioridad 4: WatchDog
  if ((vuelta % FRECUENCIA_WIFI_WATCHDOG)==0) WifiWD();    
  //------------- FIN EJECUCION DE TAREAS ---------------------------------  

  //sumo una vuelta de loop, si desborda inicializo vueltas a cero
  vuelta++;//sumo una vuelta de loop  
      
  //Espero hasta el final de la rodaja de tiempo
  while(millis()<EntradaBucle+ANCHO_INTERVALO)
    {
    if(millis()<EntradaBucle) break; //cada 49 dias el contador de millis desborda
    delay(1);
    }
  }

/********************************** Funciones de configuracion global **************************************/
/************************************************/
/* Recupera los datos de configuracion          */
/* del archivo global                           */
/************************************************/
boolean inicializaConfiguracion(boolean debug)
  {
  String cad="";
  if (debug) Serial.println("Recupero configuracion de archivo...");

  //cargo el valores por defecto
  nombre_dispositivo=String(NOMBRE_FAMILIA); //Nombre del dispositivo, por defecto el de la familia
  nivelActivo=LOW;  
  
  if(!leeFicheroConfig(GLOBAL_CONFIG_FILE, cad))
    {
    Serial.printf("No existe fichero de configuracion global\n");
    cad="{\"nombre_dispositivo\": \"" + String(NOMBRE_FAMILIA) + "\",\"NivelActivo\":0}"; //config por defecto    
    //salvo la config por defecto
    if(salvaFicheroConfig(GLOBAL_CONFIG_FILE, GLOBAL_CONFIG_BAK_FILE, cad)) Serial.printf("Fichero de configuracion global creado por defecto\n"); 
    }

  return parseaConfiguracionGlobal(cad);
  }

/*********************************************/
/* Parsea el json leido del fichero de       */
/* configuracio global                       */
/*  auto date = obj.get<char*>("date");
    auto high = obj.get<int>("high");
    auto low = obj.get<int>("low");
    auto text = obj.get<char*>("text");
/*********************************************/
boolean parseaConfiguracionGlobal(String contenido)
  {  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(contenido.c_str());
  
  //json.printTo(Serial);
  if (json.success()) 
    {
    Serial.println("parsed json");
//******************************Parte especifica del json a leer********************************
    if (json.containsKey("nombre_dispositivo")) nombre_dispositivo=((const char *)json["nombre_dispositivo"]);    
    if(nombre_dispositivo==NULL) nombre_dispositivo=String(NOMBRE_FAMILIA);
 
    if (json.containsKey("NivelActivo")) 
      {
      if((int)json["NivelActivo"]==0) nivelActivo=LOW;
      else nivelActivo=HIGH;
      }
    
    Serial.printf("Configuracion leida:\nNombre dispositivo: %s\nNivelActivo: %i\n",nombre_dispositivo.c_str(),nivelActivo);
//************************************************************************************************
    return true;
    }
  return false;  
  }

/**********************************************************************/
/* Salva la configuracion general en formato json                     */
/**********************************************************************/  
String generaJsonConfiguracionNivelActivo(String configActual, int nivelAct)
  {
  boolean nuevo=true;
  String salida="";

  if(configActual=="") 
    {
    Serial.println("No existe el fichero. Se genera uno nuevo");
    return "{\"nombre_dispositivo\": \"Nombre dispositivo\", \"NivelActivo\": \"" + String(nivelAct) + "\"}";
    }
    
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(configActual.c_str());
  json.printTo(Serial);
  if (json.success()) 
    {
    Serial.println("parsed json");          

//******************************Parte especifica del json a modificar*****************************
    json["NivelActivo"]=nivelAct;
//************************************************************************************************

    json.printTo(salida);//pinto el json que he creado
    Serial.printf("json creado:\n#%s#\n",salida.c_str());
    }//la de parsear el json

  return salida;  
  }  
