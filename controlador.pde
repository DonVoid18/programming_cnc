//DEBE SER COMPILADO UTILIZANDO EL PROGRAMA --PROCESSING--

import java.awt.event.KeyEvent;
import javax.swing.JOptionPane;
import processing.serial.*;

Serial port = null;

// Nombre del puerto para diferentes S.O(Sistema operativo)
// Dejar el nombre del puerto en null para luego seleccionar con la letra "p"
String portname = null;
//String portname = Serial.list()[0]; // Mac OS X
//String portname = "/dev/ttyUSB0"; // Linux
//String portname = "COM6"; // Windows

boolean streaming = false;//transmisión
float speed = 0.001;//velocidad
String[] gcode;
int i = 0;

void openSerialPort()  
{
  if (portname == null) return;
  if (port != null) port.stop();
  
  port = new Serial(this, portname, 9600);
  //inicializamos un objeto de tipo Serial para la comunicación con arduino
  //9600: velocidad de comunicación baudios
  
  port.bufferUntil('\n');
}

void selectSerialPort()
{
  //SELECCIONAR EL PUERTO
  String result = (String) JOptionPane.showInputDialog(frame,
    "Seleccione el puerto de serie que corresponde a su placa arduino.",
    "Seleccionar el puerto de serie",
    JOptionPane.PLAIN_MESSAGE,
    null,
    Serial.list(),
    0);
    
  if (result != null) {
    portname = result;
    openSerialPort();
  }
}

void setup()
{
  size(700, 500);
  openSerialPort();
  textSize(20);
}

void draw()
{
  background(0); 
  fill(255);
  int y = 30, dy = 25;
  text("INSTRUCCIONES PARA LA MÁQUINA CNC", 12, y); 
  y += dy;
  text("p: Seleccionar el puerto de serie", 12, y); 
  y += dy;
  text("Tecla de flecha: mover en x-y plano", 12, y); 
  y += dy;
  text("5 & 2: jog en el eje z", 12, y); 
  y += dy;
  text("$: mostrar la configuración de grbl", 12, y);
  y+= dy;
  text("h: Volver al inicio", 12, y); y += dy;
  text("0: Máquina cero (Establecer inicio en la ubicación actual)", 12, y); 
  y += dy;
  text("g: Importar un archivo de código g (.gcode)", 12, y);
  y += dy;
  text("x: Detener la ejecución del código g (Esto no es inmediato)", 12, y); 
  y += dy;
  y = height - dy;
  text("Velocidad del jog actual: " + speed + " pulgadas por paso", 12, y); 
  y -= dy;
  text("Puerto de serie actual: " + portname, 12, y); y -= dy;
}

void keyPressed()
{
  if (key == '1') speed = 0.001;
  if (key == '2') speed = 0.01;
  if (key == '3') speed = 0.1;
  
  if (!streaming) {
    if (keyCode == LEFT) port.write("G21/G90/G1 X-10  F3500\n");
    if (keyCode == RIGHT) port.write("G21/G90/G1 X10 F3500\n");
    if (keyCode == UP) port.write("G21/G90/G1 Y10 F3500\n");
    if (keyCode == DOWN) port.write("G21/G90/G1 Y-10 F3500\n");
    if (key == '5') port.write("M300 S50\n");
    if (key == '2') port.write("M300 S30\n");
    if (key == 'h') port.write("G90\nG20\nG00 X0.000 Y0.000 Z0.000\n");
    if (key == 'v') port.write("$0=75\n$1=74\n$2=75\n");
    //if (key == 'v') port.write("$0=100\n$1=74\n$2=75\n");
    if (key == 's') port.write("$3=10\n");
    if (key == 'e') port.write("$16=1\n");
    if (key == 'd') port.write("$16=0\n");
    if (key == '0') openSerialPort();
    if (key == 'p') selectSerialPort();
    if (key == '$') port.write("$$\n");
  }
  
  if (!streaming && key == 'g') {
    gcode = null; i = 0;
    File file = null; 
    println("Loading file...");
    selectInput("Select a file to process:", "fileSelected", file);
  }
  
  if (key == 'x') streaming = false;
}

void fileSelected(File selection) {
  if (selection == null) {
    println("Window was closed or the user hit cancel.");
  } else {
    println("User selected " + selection.getAbsolutePath());
    gcode = loadStrings(selection.getAbsolutePath());
    if (gcode == null) return;
    streaming = true;
    stream();
  }
}

void stream()
{
  if (!streaming) return;
  
  while (true) {
    if (i == gcode.length) {
      streaming = false;
      return;
    }
    
    if (gcode[i].trim().length() == 0) i++;
    else break;
  }
  
  println(gcode[i]);
  port.write(gcode[i] + '\n');
  i++;
}

void serialEvent(Serial p)
{
  String s = p.readStringUntil('\n');
  println(s.trim());
  
  if (s.trim().startsWith("ok")) stream();
  if (s.trim().startsWith("error")) stream(); // XXX: really?
}