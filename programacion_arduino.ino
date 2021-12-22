#include <Servo.h>
#include <AFMotor.h>
#define LINE_BUFFER_LENGTH 512
char STEP = MICROSTEP ;

// Posición del servo para arriba y abajo
const int penZUp = 70;
const int penZDown = 50;

// Yo utilizo el ping 10, pero esto depende en luegar lo conectes en el arduino
const int penServoPin =10 ;
// Revoluciones del motor de DVD
const int stepsPerRevolution = 48; 
// Creamos un objeto servo para controlar el servo
Servo penServo;

// Inicializamos los motores para los ejes x e y  (H L283D)
AF_Stepper myStepperY(stepsPerRevolution,1);
AF_Stepper myStepperX(stepsPerRevolution,2);

// Creamos variables globales
struct point {
  float x;
  float y;
  float z;
};
// posición actual del servo
struct point actuatorPos;

//  COnfiguración de dibujo, debería estar bien
float StepInc = 1;
int StepDelay = 1;
int LineDelay =0;
int penDelay = 50;

// Motor pasos para is 1 milímetro
// Usa el boceto de prueba ir 100 pasos. Mide la longitud de la linea
// Calcula pasos por mm. Entre aquí
float StepsPerMillimeterX = 100.0;
float StepsPerMillimeterY = 100.0;
// límite del robot de dibujo
// Está bien para empezar. Podría llegar hasta 50 mm si se calibra bien.
float Xmin = 0;
float Xmax = 40;
float Ymin = 0;
float Ymax = 40;
float Zmin = 0;
float Zmax = 1;
float Xpos = Xmin;
float Ypos = Ymin;
float Zpos = Zmax; 
// Establecer en verdadero para obtener la salida de depuración
boolean verbose = false;
//  necesita interpretación
//  G1 para moverse
//  G4 P300 (Espere 150ms)
//  M300 S30 (Pluma abajo)
//  M300 S50 (Pluma arriba)
//  Discard anything with a (
//  Discard any other command!
/**********************
 * void setup() - Initialisations
 ***********************/
void setup() {
  
  Serial.begin( 9600 );
  
  penServo.attach(penServoPin);
  penServo.write(penZUp);
  delay(100);
  // Disminuir si es necesario
  myStepperX.setSpeed(600);
  myStepperY.setSpeed(600);  
  
  //  Establecer y mover a la posición inicial predeterminada
  // TBD
  //  Notifications!!!
  Serial.println("Mini CNC Plotter alive and kicking!");
  Serial.print("X range is from "); 
  Serial.print(Xmin); 
  Serial.print(" to "); 
  Serial.print(Xmax); 
  Serial.println(" mm."); 
  Serial.print("Y range is from "); 
  Serial.print(Ymin); 
  Serial.print(" to "); 
  Serial.print(Ymax); 
  Serial.println(" mm."); 
}
void loop() 
{
  delay(100);
  char line[ LINE_BUFFER_LENGTH ];
  char c;
  int lineIndex;
  bool lineIsComment, lineSemiColon;
  lineIndex = 0;
  lineSemiColon = false;
  lineIsComment = false;
  while (1) {
    // Recepción en serie: principalmente de Grbl, se agregó soporte de punto y coma
    while ( Serial.available()>0 ) {
      c = Serial.read();
      if (( c == '\n') || (c == '\r') ) {             // Final de linea alcanzado
        if ( lineIndex > 0 ) {                        // La línea está completa. La ejecución!
          line[ lineIndex ] = '\0';                   // Terminar cadena
          if (verbose) { 
            Serial.print( "Received : "); 
            Serial.println( line ); 
          }
          processIncomingLine( line, lineIndex );
          lineIndex = 0;
        }
        lineIsComment = false;
        lineSemiColon = false;
        Serial.println("ok");
      }
      else {
        if ( (lineIsComment) || (lineSemiColon) ) {   // Desechar todos los caracteres de comentarios
          if ( c == ')' )  lineIsComment = false;
        }
        else {
          if ( c <= ' ' ) {                           // Desechar los espacios en blanco y los caracteres de control
          }
          else if ( c == '/' ) {
          }
          else if ( c == '(' ) {                    // Habilitación de la marca de comentarios e ignorar todos los caracteres
            lineIsComment = true;
          }
          else if ( c == ';' ) {
            lineSemiColon = true;
          }
          else if ( lineIndex >= LINE_BUFFER_LENGTH-1 ) {
            Serial.println( "ERROR - lineBuffer overflow" );
            lineIsComment = false;
            lineSemiColon = false;
          } 
          else if ( c >= 'a' && c <= 'z' ) {
            line[ lineIndex++ ] = c-'a'+'A';
          } 
          else {
            line[ lineIndex++ ] = c;
          }
        }
      }
    }
  }
}
void processIncomingLine( char* line, int charNB ) {
  int currentIndex = 0;
  char buffer[ 64 ];
  struct point newPos;
  newPos.x = 0.0;
  newPos.y = 0.0;
  while( currentIndex < charNB ) {
    switch ( line[ currentIndex++ ] ) {
    case 'U':
      penUp(); 
      break;
    case 'D':
      penDown(); 
      break;
    case 'G':
      buffer[0] = line[ currentIndex++ ]; 

      buffer[1] = '\0';
      switch ( atoi( buffer ) ){                   // seleccionar comando G
      case 0:
      case 1:
        // Supongamos que X está antes de Y
        char* indexX = strchr( line+currentIndex, 'X' );  // Obtener la posición x/y en la cadena
        char* indexY = strchr( line+currentIndex, 'Y' );
        if ( indexY <= 0 ) {
          newPos.x = atof( indexX + 1); 
          newPos.y = actuatorPos.y;
        } 
        else if ( indexX <= 0 ) {
          newPos.y = atof( indexY + 1);
          newPos.x = actuatorPos.x;
        } 
        else {
          newPos.y = atof( indexY + 1);
          indexY = '\0';
          newPos.x = atof( indexX + 1);
        }
        drawLine(newPos.x, newPos.y );
        //        Serial.println("ok");
        actuatorPos.x = newPos.x;
        actuatorPos.y = newPos.y;
        break;
      }
      break;
    case 'M':
      buffer[0] = line[ currentIndex++ ];
      buffer[1] = line[ currentIndex++ ];
      buffer[2] = line[ currentIndex++ ];
      buffer[3] = '\0';
      switch ( atoi( buffer ) ){
      case 300:
        {
          char* indexS = strchr( line+currentIndex, 'S' );
          float Spos = atof( indexS + 1);
          if (Spos == 30) { 
            penDown(); 
          }
          if (Spos == 50) { 
            penUp(); 
          }
          break;
        }
      case 114:
        Serial.print( "Absolute position : X = " );
        Serial.print( actuatorPos.x );
        Serial.print( "  -  Y = " );
        Serial.println( actuatorPos.y );
        break;
      default:
        Serial.print( "Command not recognized : M");
        Serial.println( buffer );
      }
    }
  }
}
void drawLine(float x1, float y1) {
  if (verbose)
  {
    Serial.print("fx1, fy1: ");
    Serial.print(x1);
    Serial.print(",");
    Serial.print(y1);
    Serial.println("");
  }  
  if (x1 >= Xmax) { 
    x1 = Xmax; 
  }
  if (x1 <= Xmin) { 
    x1 = Xmin; 
  }
  if (y1 >= Ymax) { 
    y1 = Ymax; 
  }
  if (y1 <= Ymin) { 
    y1 = Ymin; 
  }
  if (verbose)
  {
    Serial.print("Xpos, Ypos: ");
    Serial.print(Xpos);
    Serial.print(",");
    Serial.print(Ypos);
    Serial.println("");
  }
  if (verbose)
  {
    Serial.print("x1, y1: ");
    Serial.print(x1);
    Serial.print(",");
    Serial.print(y1);
    Serial.println("");
  }
  //  Convertir coordenadas en pasos
  x1 = (int)(x1*StepsPerMillimeterX);
  y1 = (int)(y1*StepsPerMillimeterY);
  float x0 = Xpos;
  float y0 = Ypos;
  // vamos a buscar las coordenadas
  long dx = abs(x1-x0);
  long dy = abs(y1-y0);
  int sx = x0<x1 ? StepInc : -StepInc;
  int sy = y0<y1 ? StepInc : -StepInc;
  long i;
  long over = 0;
  if (dx > dy) {
    for (i=0; i<dx; ++i) {
      myStepperX.onestep(sx,STEP);
      over+=dy;
      if (over>=dx) {
        over-=dx;
        myStepperY.onestep(sy,STEP);
      }
    delay(StepDelay);
    }
  }
  else {
    for (i=0; i<dy; ++i) {
      myStepperY.onestep(sy,STEP);
      over+=dx;
      if (over>=dy) {
        over-=dy;
        myStepperX.onestep(sx,STEP);
      }
      delay(StepDelay);
    }
  }
  if (verbose)
  {
    Serial.print("dx, dy:");
    Serial.print(dx);
    Serial.print(",");
    Serial.print(dy);
    Serial.println("");
  }
  if (verbose)
  {
    Serial.print("Going to (");
    Serial.print(x0);
    Serial.print(",");
    Serial.print(y0);
    Serial.println(")");
  }
  //  Retraso antes de que se envíen las siguientes líneas
  delay(LineDelay);
  //  Actualizamos las posiciones
  Xpos = x1;
  Ypos = y1;
}
//  subir pluma

void penUp() { 
  penServo.write(penZUp); 
  delay(penDelay); 
  Zpos=Zmax; 
  digitalWrite(15, LOW);
    digitalWrite(16, HIGH);
  if (verbose) { 
    Serial.println("Pen up!"); 
  }
}
//  bajar pluma

void penDown() { 
  penServo.write(penZDown); 
  delay(penDelay); 
  Zpos=Zmin; 
  digitalWrite(15, HIGH);
    digitalWrite(16, LOW);
  if (verbose) { 
    Serial.println("Pen down."); 
  }
}