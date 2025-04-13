/* 
 * Test sketch for getCorrectWireIndex() function
 * This allows testing the wire cutting logic without actual hardware
 */

// Wire names for debug output (copied from main sketch)
const char* wireNames[] = {
  "Roxo (fino)",      // E - Pin 3
  "Vermelho (grosso)", // F - Pin 4
  "Roxo (grosso)",    // G - Pin 5
  "Vermelho (fino)",  // H - Pin 6
  "Preto (fino)",     // A - Pin 11
  "Amarelo (grosso)", // B - Pin 12
  "Amarelo (fino)",   // C - Pin 13
  "Laranja (fino)"    // D - Pin 14
};

// Wire state tracking (1 = connected, 0 = cut)
bool wireStates[8] = {true, true, true, true, true, true, true, true};
int lastCutWireIndex = -1;  // Track the last cut wire

// Function to count uncut wires (wires that are still connected)
int countUncutWires() {
  int count = 0;
  for (int i = 0; i < 8; i++) {
    if (wireStates[i]) {
      count++;
    }
  }
  return count;
}

// Check if the wire is thick based on the wire index
bool isThickWire(int wireIndex) {
  // Thick wires: Vermelho (grosso) - F - Index 1
  //              Amarelo (grosso) - B - Index 5
  //              Roxo (grosso) - G - Index 2
  return wireIndex == 1 || wireIndex == 2 || wireIndex == 5;
}

// Count the number of thick wires that are still connected
int countUncutThickWires() {
  int count = 0;
  if (wireStates[1]) count++; // Vermelho (grosso) - F
  if (wireStates[2]) count++; // Roxo (grosso) - G
  if (wireStates[5]) count++; // Amarelo (grosso) - B
  return count;
}

// Function to determine which wire should be cut based on number of uncut wires
int getCorrectWireIndex() {
  int uncutWiresCount = countUncutWires();
  
  switch (uncutWiresCount) {
    case 8: 
      Serial.println("Entering case 8");
      return 7;  // Laranja (fino) - D - Pin 14
    
    case 7: 
      Serial.println("Entering case 7");
      // If the last cut wire was Amarelo or Laranja or Vermelho
      if (lastCutWireIndex == 5 || lastCutWireIndex == 6 || lastCutWireIndex == 7 || 
          lastCutWireIndex == 1 || lastCutWireIndex == 3) {
        // Cut the thin purple wire if there's another purple wire remaining
        if (wireStates[0] && wireStates[2]) { // If both purple wires are present
          return 0; // Cut thin purple wire (Roxo fino - E - Index 0)
        }
      } 
      
      // If the last cut wire was Preto or Roxo
      if (lastCutWireIndex == 4 || lastCutWireIndex == 0 || lastCutWireIndex == 2) {
        // Cut the thick yellow wire if it hasn't been cut yet
        if (wireStates[5]) { // If thick yellow wire exists
          return 5; // Cut thick yellow wire (Amarelo grosso - B - Index 5)
        }
      }
      
      // Otherwise (default case)
      // Cut the thin purple wire if it hasn't been cut yet, otherwise cut the thin orange wire
      if (wireStates[0]) {
        return 0; // Cut thin purple wire (Roxo fino - E - Index 0)
      } else {
        return 7; // Cut thin orange wire (Laranja fino - D - Index 7)
      }
      break;
      
    case 6: 
      Serial.println("Entering case 6");
      // Check if there's a thick purple wire and thin orange wire hasn't been cut
      if (wireStates[2] && wireStates[7]) { // Roxo (grosso) - G and Laranja (fino) - D
        return 7; // Cut thin orange wire
      }
      
      // Check if last cut was thick black or thick purple
      if (lastCutWireIndex == 2) { // Thick purple was cut last (Roxo grosso - G)
        // Cut thick light-colored wire with priority: orange > yellow > red
        // Note: We don't have a thick orange wire in the configuration
        if (wireStates[5]) return 5; // Amarelo (grosso) - B
        if (wireStates[1]) return 1; // Vermelho (grosso) - F
      }
      
      // Follow priority order
      if (wireStates[2]) return 2;       // Roxo (grosso) - G
      if (wireStates[5]) return 5;       // Amarelo (grosso) - B
      if (wireStates[4]) return 4;       // Preto (fino) - A
      if (wireStates[3]) return 3;       // Vermelho (fino) - H
      if (wireStates[7]) return 7;       // Laranja (fino) - D
      // No Laranja (grosso) in our configuration
      if (wireStates[1]) return 1;       // Vermelho (grosso) - F
      if (wireStates[6]) return 6;       // Amarelo (fino) - C
      // No Preto (grosso) in our configuration
      if (wireStates[0]) return 0;       // Roxo (fino) - E
      break;
      
    case 5:
      Serial.println("Entering case 5");
      // Count colored wires (Amarelo, Laranja, Vermelho)
      int coloredWireCount = 0;
      int darkWireCount = 0;
      
      // Count Amarelo wires
      if (wireStates[5]) coloredWireCount++; // Amarelo (grosso)
      if (wireStates[6]) coloredWireCount++; // Amarelo (fino)
      
      // Count Laranja wires
      if (wireStates[7]) coloredWireCount++; // Laranja (fino)
      // No Laranja (grosso) in our configuration
      
      // Count Vermelho wires
      if (wireStates[1]) coloredWireCount++; // Vermelho (grosso)
      if (wireStates[3]) coloredWireCount++; // Vermelho (fino)
      
      // Count Preto and Roxo wires
      // No Preto (grosso) in our configuration
      if (wireStates[4]) darkWireCount++; // Preto (fino)
      if (wireStates[0]) darkWireCount++; // Roxo (fino)
      if (wireStates[2]) darkWireCount++; // Roxo (grosso)
      
      // Rule 1: More colored than dark wires
      if (coloredWireCount > darkWireCount && wireStates[6]) {
        return 6; // Cut Amarelo (fino) - C
      }
      
      // Rule 2: Odd number of thick wires
      int thickWiresCount = countUncutThickWires();
      if (thickWiresCount % 2 == 1 && wireStates[3]) {
        return 3; // Cut Vermelho (fino) - H
      }
      
      // Rule 3: Cut based on priority sequence
      // Preto Grosso < Roxo Fino < Laranja Grosso < Amarelo Fino < Vermelho Grosso 
      // < Laranja Fino < Amarelo Grosso < Vermelho Fino < Roxo Grosso < Preto Fino
      
      // No Preto (grosso) in our configuration
      if (wireStates[0]) return 0; // Roxo (fino) - E
      // No Laranja (grosso) in our configuration
      if (wireStates[6]) return 6; // Amarelo (fino) - C
      if (wireStates[1]) return 1; // Vermelho (grosso) - F
      if (wireStates[7]) return 7; // Laranja (fino) - D
      if (wireStates[5]) return 5; // Amarelo (grosso) - B
      if (wireStates[3]) return 3; // Vermelho (fino) - H
      if (wireStates[2]) return 2; // Roxo (grosso) - G
      if (wireStates[4]) return 4; // Preto (fino) - A
      break;
      
    case 4:
      Serial.println("Entering case 4");
      // Check if last cut wire was Yellow (Amarelo)
      if (lastCutWireIndex == 5 || lastCutWireIndex == 6) { // Amarelo (grosso or fino)
        // Check if there are 2 or more thick wires left
        int thickWiresCount = countUncutThickWires();
        if (thickWiresCount >= 2 && wireStates[2]) {
          return 2; // Cut Roxo (grosso) - G if available
        }
        
        // Check for exactly one pair of wires of the same color
        bool hasRedPair = wireStates[1] && wireStates[3]; // Vermelho pair
        bool hasPurplePair = wireStates[0] && wireStates[2]; // Roxo pair
        bool hasYellowPair = wireStates[5] && wireStates[6]; // Amarelo pair
        
        // Count total pairs
        int pairCount = 0;
        if (hasRedPair) pairCount++;
        if (hasPurplePair) pairCount++;
        if (hasYellowPair) pairCount++;
        
        // If there's exactly one pair, cut the thin wire of that pair
        if (pairCount == 1) {
          if (hasRedPair) return 3;    // Cut Vermelho (fino) - H
          if (hasPurplePair) return 0; // Cut Roxo (fino) - E
          if (hasYellowPair) return 6; // Cut Amarelo (fino) - C
        }
      }
      
      // If last wire cut was black or purple
      else if (lastCutWireIndex == 0 || lastCutWireIndex == 2 || lastCutWireIndex == 4) {
        // Priority: Laranja Fino > Amarelo Grosso > Vermelho Fino > Laranja Grosso > Vermelho Grosso > Amarelo Fino
        if (wireStates[7]) return 7; // Laranja (fino) - D
        if (wireStates[5]) return 5; // Amarelo (grosso) - B
        if (wireStates[3]) return 3; // Vermelho (fino) - H
        // No Laranja (grosso) in our configuration
        if (wireStates[1]) return 1; // Vermelho (grosso) - F
        if (wireStates[6]) return 6; // Amarelo (fino) - C
      }
      
      // If last wire cut was orange, yellow or red
      else if (lastCutWireIndex == 1 || lastCutWireIndex == 3 || lastCutWireIndex == 5 || 
               lastCutWireIndex == 6 || lastCutWireIndex == 7) {
        // Priority: Roxo Grosso > Preto Fino > Preto Grosso > Roxo Fino
        if (wireStates[2]) return 2; // Roxo (grosso) - G
        if (wireStates[4]) return 4; // Preto (fino) - A
        // No Preto (grosso) in our configuration
        if (wireStates[0]) return 0; // Roxo (fino) - E
      }
      
      // Default rule - separate by color
      if (lastCutWireIndex == 0 || lastCutWireIndex == 2 || lastCutWireIndex == 4) {
        // If last cut was black or purple, cut in this order: Grosso Roxo > Grosso Preto > Fino Preto > Fino Roxo
        if (wireStates[2]) return 2; // Roxo (grosso) - G
        // No Preto (grosso) in our configuration
        if (wireStates[4]) return 4; // Preto (fino) - A
        if (wireStates[0]) return 0; // Roxo (fino) - E
      } else {
        // If last cut was orange, yellow or red, cut in this order:
        // Grosso Amarelo > Grosso Laranja > Grosso Vermelho > Fino Vermelho > Fino Laranja > Fino Amarelo
        if (wireStates[5]) return 5; // Amarelo (grosso) - B
        // No Laranja (grosso) in our configuration
        if (wireStates[1]) return 1; // Vermelho (grosso) - F
        if (wireStates[3]) return 3; // Vermelho (fino) - H
        if (wireStates[7]) return 7; // Laranja (fino) - D
        if (wireStates[6]) return 6; // Amarelo (fino) - C
      }
      break;
      
    case 3:
      Serial.println("Entering case 3");
      // Check if the remaining wires are Yellow, Red, and any other color
      bool hasYellow = wireStates[5] || wireStates[6]; // Amarelo (grosso or fino)
      bool hasRed = wireStates[1] || wireStates[3];    // Vermelho (grosso or fino)
      
      // Count total remaining wires by color
      int yellowCount = (wireStates[5] ? 1 : 0) + (wireStates[6] ? 1 : 0);
      int redCount = (wireStates[1] ? 1 : 0) + (wireStates[3] ? 1 : 0);
      int purpleCount = (wireStates[0] ? 1 : 0) + (wireStates[2] ? 1 : 0);
      int blackCount = (wireStates[4] ? 1 : 0); // Only Preto (fino) exists
      int orangeCount = (wireStates[7] ? 1 : 0); // Only Laranja (fino) exists
      
      // If we have Yellow, Red, and exactly one other color
      if (hasYellow && hasRed && (yellowCount + redCount == 2) && (purpleCount + blackCount + orangeCount == 1)) {
        // Cut the other color
        if (purpleCount > 0) {
          if(wireStates[0]) return 0; // Cut Roxo (fino) - E
          else if (wireStates[2]) return 2; // Cut Roxo (grosso) - G
        }
        if (blackCount > 0 && wireStates[4]) {
          return 4; // Return Preto (fino)
        }
        if (orangeCount > 0 && wireStates[7]) {
          return 7; // Return Laranja (fino)
        }
      }
      
      // If last cut wire was Black or Purple
      if (lastCutWireIndex == 0 || lastCutWireIndex == 2 || lastCutWireIndex == 4) {
        // Count thin versus thick wires
        int thinWiresCount = 0;
        int thickWiresCount = 0;
        
        // Count thin wires
        if (wireStates[0]) thinWiresCount++; // Roxo (fino)
        if (wireStates[3]) thinWiresCount++; // Vermelho (fino)
        if (wireStates[4]) thinWiresCount++; // Preto (fino)
        if (wireStates[6]) thinWiresCount++; // Amarelo (fino)
        if (wireStates[7]) thinWiresCount++; // Laranja (fino)
        
        // Count thick wires
        if (wireStates[1]) thickWiresCount++; // Vermelho (grosso)
        if (wireStates[2]) thickWiresCount++; // Roxo (grosso)
        if (wireStates[5]) thickWiresCount++; // Amarelo (grosso)
        
        // If more thin wires than thick wires
        if (thinWiresCount > thickWiresCount && wireStates[7]) {
          return 7; // Cut Laranja (fino)
        }
      }
      
      // If last cut wire was thick
      if (isThickWire(lastCutWireIndex)) {
        // Cut in sequence: Vermelho Grosso > Preto Fino > Amarelo Grosso > Roxo Fino > Laranja Grosso > Vermelho Fino > Preto Grosso > Roxo Grosso > Amarelo Fino > Laranja Fino
        if (wireStates[1]) return 1; // Vermelho (grosso) - F
        if (wireStates[4]) return 4; // Preto (fino) - A
        if (wireStates[5]) return 5; // Amarelo (grosso) - B
        if (wireStates[0]) return 0; // Roxo (fino) - E
        // No Laranja (grosso) in our configuration
        if (wireStates[3]) return 3; // Vermelho (fino) - H
        // No Preto (grosso) in our configuration
        if (wireStates[2]) return 2; // Roxo (grosso) - G
        if (wireStates[6]) return 6; // Amarelo (fino) - C
        if (wireStates[7]) return 7; // Laranja (fino) - D
      }
      // If last cut wire was thin
      else {
        // Cut in sequence: Vermelho Grosso > Vermelho Fino > Roxo Fino > Roxo Gross > Preto Grosso > Preto Fino > Laranja Grosso > Laranja Fino > Amarelo Fino > Amarelo Grosso
        if (wireStates[1]) return 1; // Vermelho (grosso) - F
        if (wireStates[3]) return 3; // Vermelho (fino) - H
        if (wireStates[0]) return 0; // Roxo (fino) - E
        if (wireStates[2]) return 2; // Roxo (grosso) - G
        // No Preto (grosso) in our configuration
        if (wireStates[4]) return 4; // Preto (fino) - A
        // No Laranja (grosso) in our configuration
        if (wireStates[7]) return 7; // Laranja (fino) - D
        if (wireStates[6]) return 6; // Amarelo (fino) - C
        if (wireStates[5]) return 5; // Amarelo (grosso) - B
      }
      break;
      
    case 2:
      Serial.println("Entering case 2");
      // Store the indexes of the remaining two wires
      int remainingWires[2];
      int wireCount = 0;
      
      for (int i = 0; i < 8; i++) {
        if (wireStates[i]) {
          remainingWires[wireCount++] = i;
          if (wireCount >= 2) break; // Found both wires
        }
      }
      
      bool wire1IsColoredWire = remainingWires[0] == 1 || remainingWires[0] == 3 || // Vermelho
                               remainingWires[0] == 5 || remainingWires[0] == 6 || // Amarelo
                               remainingWires[0] == 7; // Laranja
                               
      bool wire2IsColoredWire = remainingWires[1] == 1 || remainingWires[1] == 3 || // Vermelho
                               remainingWires[1] == 5 || remainingWires[1] == 6 || // Amarelo
                               remainingWires[1] == 7; // Laranja
                               
      bool wire1IsDarkWire = remainingWires[0] == 0 || remainingWires[0] == 2 || // Roxo
                            remainingWires[0] == 4; // Preto
                            
      bool wire2IsDarkWire = remainingWires[1] == 0 || remainingWires[1] == 2 || // Roxo
                            remainingWires[1] == 4; // Preto
      
      // Check if both are colored (yellow, red, orange)
      if (wire1IsColoredWire && wire2IsColoredWire) {
        // If one is thick and one is thin, cut the thin one
        bool wire1IsThick = isThickWire(remainingWires[0]);
        bool wire2IsThick = isThickWire(remainingWires[1]);
        
        if (wire1IsThick && !wire2IsThick) {
          return remainingWires[1]; // Cut the thin wire (wire2)
        }
        else if (!wire1IsThick && wire2IsThick) {
          return remainingWires[0]; // Cut the thin wire (wire1)
        }
      }
      
      // Check if both are dark (black or purple)
      if (wire1IsDarkWire && wire2IsDarkWire) {
        // If one is thick and one is thin, cut the thin one
        bool wire1IsThick = isThickWire(remainingWires[0]);
        bool wire2IsThick = isThickWire(remainingWires[1]);
        
        if (wire1IsThick && !wire2IsThick) {
          return remainingWires[1]; // Cut the thin wire (wire2)
        }
        else if (!wire1IsThick && wire2IsThick) {
          return remainingWires[0]; // Cut the thin wire (wire1)
        }
      }
      
      // If last cut wire was yellow, red, or orange
      if (lastCutWireIndex == 1 || lastCutWireIndex == 3 || // Vermelho
          lastCutWireIndex == 5 || lastCutWireIndex == 6 || // Amarelo
          lastCutWireIndex == 7) { // Laranja
        
        // Check for black wires
        bool hasBlackWire = wireStates[4]; // Preto fino
        // No Preto grosso in our configuration
        
        if (hasBlackWire) {
          return 4; // Cut the black wire
        }
        
        // Check for purple wires
        bool hasThinPurple = wireStates[0]; // Roxo fino
        bool hasThickPurple = wireStates[2]; // Roxo grosso
        
        if (hasThinPurple && !hasThickPurple) {
          return 0; // Cut thin purple
        }
        else if (!hasThinPurple && hasThickPurple) {
          return 2; // Cut thick purple
        }
      }
      
      // If last cut wire was black or purple
      if (lastCutWireIndex == 0 || lastCutWireIndex == 2 || // Roxo
          lastCutWireIndex == 4) { // Preto
        
        // Count colored wires
        int coloredWireCount = 0;
        if (wireStates[1]) coloredWireCount++; // Vermelho grosso
        if (wireStates[3]) coloredWireCount++; // Vermelho fino
        if (wireStates[5]) coloredWireCount++; // Amarelo grosso
        if (wireStates[6]) coloredWireCount++; // Amarelo fino
        if (wireStates[7]) coloredWireCount++; // Laranja fino
        
        // If only one colored wire left
        if (coloredWireCount == 1) {
          if (wireStates[1]) return 1; // Vermelho grosso (thick)
          if (wireStates[5]) return 5; // Amarelo grosso (thick)
          // No Laranja grosso in our configuration
          
          // If no thick colored wires, cut the thin colored wire
          if (wireStates[3]) return 3; // Vermelho fino
          if (wireStates[6]) return 6; // Amarelo fino
          if (wireStates[7]) return 7; // Laranja fino
        }
      }
      
      // Otherwise cut the thin wire following the sequence: Amarelo > Preto > Vermelho > Laranja > Roxo
      if (wireStates[6]) return 6; // Amarelo fino
      if (wireStates[4]) return 4; // Preto fino
      if (wireStates[3]) return 3; // Vermelho fino
      if (wireStates[7]) return 7; // Laranja fino
      if (wireStates[0]) return 0; // Roxo fino
      
      // Otherwise cut the thick wire following the sequence: Vermelho > Preto > Amarelo > Laranja > Roxo
      if (wireStates[1]) return 1; // Vermelho grosso
      // No Preto grosso in our configuration
      if (wireStates[5]) return 5; // Amarelo grosso
      // No Laranja grosso in our configuration
      if (wireStates[2]) return 2; // Roxo grosso
      break;
      
    case 1:
      Serial.println("Entering case 1");
      // If only one wire is left, cut it
      for (int i = 0; i < 8; i++) {
        if (wireStates[i]) {
          return i; // Cut the last remaining wire
        }
      }
      break;

    default: return -1;
  }
}


// Print the current state of all wires
void printWireStates() {
  Serial.println("\n--- Estado dos fios ---");
  for (int i = 0; i < 8; i++) {
    Serial.print(wireNames[i]);
    Serial.print(": ");
    Serial.println(wireStates[i] ? "CONECTADO" : "CORTADO");
  }
  
  // Print as an array
  Serial.print("Array de fios [");
  for (int i = 0; i < 8; i++) {
    Serial.print(wireStates[i] ? "1" : "0");
    if (i < 7) Serial.print(", ");
  }
  Serial.println("]");
  
  int correctWireIndex = getCorrectWireIndex();
  if (correctWireIndex >= 0) {
    Serial.print("Próximo fio a ser cortado: ");
    Serial.println(wireNames[correctWireIndex]);
    Serial.print("Fios restantes: ");
    Serial.println(countUncutWires());
  } else {
    Serial.println("Nenhum fio correto encontrado.");
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Test for getCorrectWireIndex() function");
  Serial.println("Commands:");
  Serial.println("  c[0-7] - Cut a wire (example: c3 cuts wire 3)");
  Serial.println("  r[0-7] - Reconnect a wire (example: r3 reconnects wire 3)");
  Serial.println("  last[0-7] - Set last cut wire (example: last3 sets wire 3 as last cut)");
  Serial.println("  reset - Reset all wires to connected");
  Serial.println("  state - Print current wire states");
  
  // Print initial state
  printWireStates();
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.startsWith("c") && command.length() == 2) {
      int wireIndex = command.substring(1).toInt();
      if (wireIndex >= 0 && wireIndex < 8) {
        wireStates[wireIndex] = false;
        lastCutWireIndex = wireIndex;
        Serial.print("Cut wire: ");
        Serial.println(wireNames[wireIndex]);
        printWireStates();
      }
    }
    else if (command.startsWith("r") && command.length() == 2) {
      int wireIndex = command.substring(1).toInt();
      if (wireIndex >= 0 && wireIndex < 8) {
        wireStates[wireIndex] = true;
        Serial.print("Reconnected wire: ");
        Serial.println(wireNames[wireIndex]);
        printWireStates();
      }
    }
    else if (command.startsWith("last") && command.length() == 5) {
      int wireIndex = command.substring(4).toInt();
      if (wireIndex >= 0 && wireIndex < 8) {
        lastCutWireIndex = wireIndex;
        Serial.print("Set last cut wire to: ");
        Serial.println(wireNames[wireIndex]);
        printWireStates();
      }
    }
    else if (command == "reset") {
      for (int i = 0; i < 8; i++) {
        wireStates[i] = true;
      }
      lastCutWireIndex = -1;
      Serial.println("Reset all wires to connected");
      printWireStates();
    }
    else if (command == "state") {
      printWireStates();
    }
    else {
      Serial.println("Unknown command");
    }
  }
}
