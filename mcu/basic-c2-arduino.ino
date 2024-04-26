/* contains splitted portions of c2gen (simple c2 signal driver for arduino) */

#define C2D_PIN  10
#define C2CK_PIN 11
#define LED_PIN  13

#define DEVID 	0x17  	// Family (DEVID)
#define FPDAT 	0xb4 	// FPDAT Address
#define PSIZE	512 	// Page Size
#define FPCTL	0x02 	// FPCTL = DEVCTL ? seems like it
#define EPCTL   0xdf
#define EPDAT   0xbf
#define EPSTAT  0xb7
#define EPADDRH 0xaf
#define EPADDRL 0xae

#define CRC0 	0xa9
#define CRC1 	0xaa
#define CRC2 	0xab
#define CRC3 	0xac

void setup()
{
  pinMode(LED_PIN, OUTPUT);  // led
  pinMode(C2CK_PIN, OUTPUT);  // clock
  pinMode(12, OUTPUT);  // clock

  Serial.begin(115200);
}

void oscillator_initialization()
{
	/* AN127, p.28 : WriteDirect(0xB2, 0x83) */
	/* from what I extrapolate from c2tool: */

	// only found in c2tool:
	write_sfr(0xcf, 0x00); // unsure it changes anything.

	// this is coherent with AN127
	write_sfr(0xb2, 0x83);
}

void initialize_pi()
{
	/* AN127 p.15 */
	/*PI must be initialized with the following sequence*/
	c2ck_low();              //1. drive C2CK low
	delayMicroseconds(20+2); //2. wait 20µs at least
	c2ck_high();             //3. drive C2CK high
	delayMicroseconds(2+2);  //4. wait 2µs at least
      	address_write(FPCTL);     //5. perform address write instruction targeting FPCTL register (0x02)
	data_write(0x02);        //6. perform data write instruction sending a value of 0x02
	data_write(0x04);        //7. perform data write instruction sending a value of 0x04 to halt the core
	data_write(0x01);        //8. perform data write instruction sending a value of 0x01

	delayMicroseconds(20+2); //9. wait 20µs at least

	////* interpolating something: p13. EPCTL  */
	address_write(EPCTL);
	data_write(0x40);
	data_write(0x58);

}

void reset() {
	PORTB = PORTB | B00001000;     // pin 11 (clock)
	delayMicroseconds(2);
	PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
	delayMicroseconds(20);
	PORTB = PORTB | B00001000;     // pin 11 (clock)
	delayMicroseconds(2);
}

/* Common functions : AN127 p.30 */
void write_sfr(unsigned char address, unsigned char data)
{
	address_write(address);
	data_write(data);
}

void read_sfr(unsigned char address)
{
	address_write(address);
	data_read();
}

void write_command(unsigned char command)
{
	data_write(command);
	poll_inbusy();
}

void read_data()
{
	poll_outready();
	data_read();
}

void write_direct(unsigned char address, unsigned char data)
{
	address_write(FPDAT);
	write_command(0x0a); // direct write
	//Serial.println("0x0d would be a success:");
	read_data(); // 0x0d indicates success, all other return values are error
	write_command(address);
	write_command(0x01);
	write_command(data);
}

void read_direct(unsigned char address)
{
	address_write(FPDAT);
	write_command(0x09);
	read_data(); // 0x0d indicates success, all other return values are errors
	write_command(address);
	write_command(0x01);
	read_data();
}

/* */
void poll_inbusy()
{
	unsigned char address;
	int timeout = 20;
	while (--timeout > 0) {
		if (! (address & 0x02)) {
			break;
		}
		delayMicroseconds(1);
	}
	if (timeout == 0) {
		return -1;
	}
}

void poll_outready()
{
	unsigned char address;
	int timeout = 100000;
	while (--timeout > 0) {
		if ( address & 0x01 ) {
			break;
		}
		delayMicroseconds(1);
	}
	if (timeout == 0) {
		return -1;
	}
}


/* */
void data_read() {
	char ret;
	int t, i;
	unsigned char udata;
        ret = '0';
        pinMode(C2D_PIN, OUTPUT);
        /* Start field */
        PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

        /* Ins field */
        PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)
//        PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

        /* Length field */
//        PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)
//        PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

        /* Wait field */
        PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
        t = 40;
        pinMode(C2D_PIN, INPUT);
        while (t>0) {
          PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
          PORTB = PORTB | B00001000;     // pin 11 (clock)
          if (digitalRead(C2D_PIN))
            break;
          delayMicroseconds(1);
          t--;
          if (t == 0) {
            ret = '1';
            break;
          }
        }

        /* Data field */
        udata = 0;
        for (i = 0; i < 8; i++) {
          udata = udata >> 1;
          PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
          PORTB = PORTB | B00001000;     // pin 11 (clock)
//          delayMicroseconds(100);
          if (digitalRead(C2D_PIN))
            udata = udata | 0x80;
        }
        /* Stop field */
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

        //Serial.print((char)(udata));
        //Serial.print(ret);

	//Serial.println("----");
	//Serial.print("hexa: ");
	//Serial.print(udata, HEX);
	//Serial.print("  ; binary: ");
	//Serial.print(udata, BIN);
	//Serial.print("  ; decimal: ");
	//Serial.print(udata, DEC);
	//Serial.println("");

	Serial.println(udata, HEX);
}

void data_read_epstat() {
	char ret;
	int t, i;
	unsigned char udata;
        ret = '0';
        pinMode(C2D_PIN, OUTPUT);
        /* Start field */
        PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

        /* Ins field */
        PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)
//        PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

        /* Length field */
//        PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)
//        PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

        /* Wait field */
        PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
        t = 40;
        pinMode(C2D_PIN, INPUT);
        while (t>0) {
          PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
          PORTB = PORTB | B00001000;     // pin 11 (clock)
          if (digitalRead(C2D_PIN))
            break;
          delayMicroseconds(1);
          t--;
          if (t == 0) {
            ret = '1';
            break;
          }
        }

        /* Data field */
        udata = 0;
        for (i = 0; i < 8; i++) {
          udata = udata >> 1;
          PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
          PORTB = PORTB | B00001000;     // pin 11 (clock)
//          delayMicroseconds(100);
          if (digitalRead(C2D_PIN))
            udata = udata | 0x80;
        }
        /* Stop field */
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

        //Serial.print((char)(udata));
        //Serial.print(ret);

	//Serial.println("----");
	//Serial.print("hexa: ");
	//Serial.print(udata, HEX);
	//Serial.print("  ; binary: ");
	//Serial.print(udata, BIN);
	//Serial.print("  ; decimal: ");
	//Serial.print(udata, DEC);
	//Serial.println("");

	Serial.println("");
	Serial.print("  > ");
	Serial.print(udata, BIN);
	Serial.println("");
}

void c2ck_high() {
        PORTB = PORTB | B00001000;  // 100% sure
}

void c2ck_low() {
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)  (digitalWrite Low)
}

void address_read()
{
  	unsigned char udata;
  	int i;
        /* read_ar */
	//pinMode(C2D_PIN, OUTPUT);
        DDRB =   B00101100;
        /* Start field */
        PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

        /* Ins field */
        PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)
        PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

        /* Adress field */
	//PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
        udata = 0;
	//pinMode(C2D_PIN, INPUT);
        DDRB =  B00101000;
        for (i = 0; i < 8; i++) {
          udata = udata >> 1;
          PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
          PORTB = PORTB | B00001000;     // pin 11 (clock)
          delayMicroseconds(1);
	  //if (digitalRead(C2D_PIN))
          if (PINB & B00000100)
            udata = udata | 0x80;
        }

        /* Stop field */
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

        //Serial.print((char)udata);
}

void address_write(unsigned char address)
{
	//unsigned char address;
	int i;

	/*C2CK Driver On = C2CK High ('C') */
        PORTB = PORTB | B00001000;  // 100% sure

	/*Strobe C2CK (start field)*/
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

	/*C2D Driver On = C2D High ('O') */
        pinMode(C2D_PIN, OUTPUT);
        PORTB = PORTB | B00100100;     // not sure

	/*Force C2D high*/
	// howto??
        PORTB = PORTB | B00100100; // not sure again...

	/*Strobe 11b instruction field */
        PORTB = PORTB & ~(B00001000);  // 1  (first strobe)
        PORTB = PORTB | B00001000;     //
        PORTB = PORTB & ~(B00001000);  // 1  (second strobe)
        PORTB = PORTB | B00001000;     //

// LOAD THE DEVCTL REGISTER

	/*Force C2D to each bit of the address (starting with bit 0) and strobe for each bit*/
        //address = 0x02;

        for (i = 0; i < 8; i++) {
          if (address & 0x01) {
            PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
            PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
            PORTB = PORTB | B00001000;     // pin 11 (clock)
            //Serial.print('1');
          } else {
            PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
            PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
            PORTB = PORTB | B00001000;     // pin 11 (clock)
            //Serial.print('0');
          }
          address = address >> 1;
        }

	/*Turn off C2D driver ('o')*/
        pinMode(C2D_PIN, OUTPUT);
        PORTB = PORTB & ~(B00100100);  // pin 10 (data+led)

	/*Strobe to Stop field*/
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)
}

void data_write(unsigned char data)
{
	int t, i;

        pinMode(C2D_PIN, OUTPUT);
        /* Start field */
        PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

        /* Ins field */
//        PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)
        PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

        /* Length field */
//        PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)
//        PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

        /* Data field */
        //do {
        //} while (Serial.available() == 0);
        //data = (char)Serial.read();


        for (i = 0; i < 8; i++) {
          if ((unsigned char)data & 1) {
            PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
            PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
            PORTB = PORTB | B00001000;     // pin 11 (clock)
//            Serial.print('1');
          } else {
            PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
            PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
            PORTB = PORTB | B00001000;     // pin 11 (clock)
//            Serial.print('0');
          }
          data = (unsigned char) data >> 1;
        }

        /* Wait field */
        PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
        t = 40;
        pinMode(C2D_PIN, INPUT);
        while (t>0) {
          PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
          PORTB = PORTB | B00001000;     // pin 11 (clock)
          if (digitalRead(C2D_PIN))
            break;
          delayMicroseconds(1);
          t--;
          //if (t == 0)
          //  Serial.print('1');
        }

        /* Stop field */
        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        PORTB = PORTB | B00001000;     // pin 11 (clock)

        //Serial.print('0');
}

void loop()
{
  char c2d, inChar, t, data, ret;
  unsigned char udata;
  unsigned char udata_str[2];
  int i;

  //initialize_pi();
  //oscillator_initialization();

  // read serial input
  if (Serial.available() > 0) {
    inChar = (char)Serial.read();

    // handle commands
    switch(inChar) {

      case 'g': // Get device id (returns 17)
      	{
		reset();
		data_read();
        	break;
	}

      case 'i': // Get data
      	{
        	pinMode(C2D_PIN, INPUT);
        	c2d = digitalRead(C2D_PIN) + '0';
        	Serial.print(c2d);
        	break;
	}

      case 'o':
      	{
        	pinMode(C2D_PIN, OUTPUT);
        	PORTB = PORTB & ~(B00100100);  // pin 10 (data+led)
        	break;
	}

      case 'O': // C2D driver on?
      	{
        	pinMode(C2D_PIN, OUTPUT);
        	PORTB = PORTB | B00100100;     // pin 10 (data+led)
        	break;
	}

      case 's':
      	{
        	PORTB = PORTB | B00001000;     // pin 11 (clock)
        	/* strobe pulse low for 250nS with this code */
        	PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        	PORTB = PORTB | B00001000;     // pin 11 (clock)
        	break;
	}

      case 'r':
      	{
        	PORTB = PORTB | B00001000;     // pin 11 (clock)
        	delayMicroseconds(2);
        	PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        	delayMicroseconds(20);
        	PORTB = PORTB | B00001000;     // pin 11 (clock)
        	delayMicroseconds(2);
        	break;
	}

      case 'D':
      	{
	        /* write_dr */
	        pinMode(C2D_PIN, OUTPUT);
	        /* Start field */
	        PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
	        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
	        PORTB = PORTB | B00001000;     // pin 11 (clock)

	        /* Ins field */
		//PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
	        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
	        PORTB = PORTB | B00001000;     // pin 11 (clock)
	        PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
	        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
	        PORTB = PORTB | B00001000;     // pin 11 (clock)

	        /* Length field */
		//PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
	        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
	        PORTB = PORTB | B00001000;     // pin 11 (clock)
		//PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
	        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
	        PORTB = PORTB | B00001000;     // pin 11 (clock)

	        /* Data field */
	        do {
	        } while (Serial.available() == 0);
	        data = (char)Serial.read();


	        for (i = 0; i < 8; i++) {
	          if ((unsigned char)data & 1) {
	            PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
	            PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
	            PORTB = PORTB | B00001000;     // pin 11 (clock)
		    //Serial.print('1');
	          } else {
	            PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
	            PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
	            PORTB = PORTB | B00001000;     // pin 11 (clock)
		    //Serial.print('0');
	          }
	          data = (unsigned char) data >> 1;
	        }

	        /* Wait field */
	        PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
	        t = 40;
	        pinMode(C2D_PIN, INPUT);
	        while (t>0) {
	          PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
	          PORTB = PORTB | B00001000;     // pin 11 (clock)
	          if (digitalRead(C2D_PIN))
	            break;
	          delayMicroseconds(1);
	          t--;
	          if (t == 0)
	            Serial.print('1');
	        }

	        /* Stop field */
	        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
	        PORTB = PORTB | B00001000;     // pin 11 (clock)

	        Serial.print('0');
	        break;
	}

      case 'A':
      	{
	        /* read_ar */
		//pinMode(C2D_PIN, OUTPUT);
	        DDRB =   B00101100;
	        /* Start field */
	        PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
	        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
	        PORTB = PORTB | B00001000;     // pin 11 (clock)

	        /* Ins field */
	        PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
	        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
	        PORTB = PORTB | B00001000;     // pin 11 (clock)
	        PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
	        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
	        PORTB = PORTB | B00001000;     // pin 11 (clock)

	        /* Adress field */
		//PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
	        udata = 0;
		//pinMode(C2D_PIN, INPUT);
	        DDRB =  B00101000;
	        for (i = 0; i < 8; i++) {
	          udata = udata >> 1;
	          PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
	          PORTB = PORTB | B00001000;     // pin 11 (clock)
	          delayMicroseconds(1);
		  //if (digitalRead(C2D_PIN))
	          if (PINB & B00000100)
	            udata = udata | 0x80;
	        }

	        /* Stop field */
	        PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
	        PORTB = PORTB | B00001000;     // pin 11 (clock)

	        Serial.print((char)udata);
	        break;
	}

      case 'd':
      	{
        	/* read dr */
        	ret = '0';
        	pinMode(C2D_PIN, OUTPUT);
        	/* Start field */
        	PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
        	PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        	PORTB = PORTB | B00001000;     // pin 11 (clock)

        	/* Ins field */
        	PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        	PORTB = PORTB & ~(B00001000);  // pin 11 (clock) <- strobe
        	PORTB = PORTB | B00001000;     // pin 11 (clock)
//      	  PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        	PORTB = PORTB & ~(B00001000);  // pin 11 (clock) <- strobe
        	PORTB = PORTB | B00001000;     // pin 11 (clock)

        	/* Length field */
//      	  PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        	PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        	PORTB = PORTB | B00001000;     // pin 11 (clock)
//      	  PORTB = PORTB & ~(B00100100);  // 0 -> pin 10 (data+led)
        	PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        	PORTB = PORTB | B00001000;     // pin 11 (clock)

        	/* Wait field */
        	PORTB = PORTB | B00100100;     // 1 -> pin 10 (data+led)
        	t = 40;
        	pinMode(C2D_PIN, INPUT);
        	while (t>0) {
        	  PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        	  PORTB = PORTB | B00001000;     // pin 11 (clock)
        	  if (digitalRead(C2D_PIN))
        	    break;
        	  delayMicroseconds(1);
        	  t--;
        	  if (t == 0) {
        	    ret = '1';
        	    break;
        	  }
        	}

        	/* Data field */
        	udata = 0;
        	for (i = 0; i < 8; i++) {
        	  udata = udata >> 1;
        	  PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        	  PORTB = PORTB | B00001000;     // pin 11 (clock)
//      	    delayMicroseconds(100);
        	  if (digitalRead(C2D_PIN))
        	    udata = udata | 0x80;
        	}
        	/* Stop field */
        	PORTB = PORTB & ~(B00001000);  // pin 11 (clock)
        	PORTB = PORTB | B00001000;     // pin 11 (clock)

        	Serial.print((char)(udata));
        	Serial.print(ret);
        	break;
	}

      case 'm':
      	{
      		Serial.println("Simple C2 interface for C8051T634");
		break;
	}

      case 'C':
      	{
      		Serial.println("Performing 32-bit CRCs on Full EPROM Content (polynomial 0x04C11DB7)");
		/*
		A 32-bit CRC on the entire EPROM space is initiated by writing to the CRC1 byte over the C2 interface.
		The CRC calculation begins at address 0x0000 and ends at the end of user EPROM space. The EPBusy
		bit in register C2ADD will be set during the CRC operation, and cleared once the operation is complete.
		The 32-bit results will be available in the CRC3-0 registers. CRC3 is the MSB, and CRC0 is the LSB. The
		polynomial used for the 32-bit CRC calculation is 0x04C11DB7.
		Note: If a 16-bit CRC has been performed since the last device reset, a device reset should be initiated before
		performing a 32-bit CRC operation.
		*/

		write_sfr(CRC1, 0x0000);
		//delayMicroseconds(10000);
		read_sfr(CRC3);
		read_sfr(CRC2);
		read_sfr(CRC1);
		read_sfr(CRC0);
		break;
	}

      case 'c':
      	{
      		Serial.println("Performing 16-bit CRC (polynomial 0x1021)");
		/*
		A 16-bit CRC of individual 256-byte blocks of EPROM can be initiated by writing to the CRC0 byte over the
		C2 interface. The value written to CRC0 is the high byte of the beginning address for the CRC. For exam-
		ple, if CRC0 is written to 0x02, the CRC will be performed on the 256-bytes beginning at address 0x0200,
		and ending at address 0x2FF. The EPBusy bit in register C2ADD will be set during the CRC operation, and
		cleared once the operation is complete. The 16-bit results will be available in the CRC1-0 registers. CRC1
		is the MSB, and CRC0 is the LSB. The polynomial for the 16-bit CRC calculation is 0x1021
		*/

		write_sfr(CRC0, 0x00);
		//delayMicroseconds(10000);
		read_sfr(CRC1);
		read_sfr(CRC0);
		break;
	}

      /* DUMP EPROM from 0x0000 to 0x0800 */
      case 'e':
        {
  		initialize_pi();
  		oscillator_initialization();
        	//unsigned char address = 0x0000; // eprom first address
        	unsigned char address = 0x0000; // eprom adress i want to read

      		// IMPLEMENTATION OF THE ADDRESS READ PROCEDURE
		////1.  reset the device /RST
		//reset();

		////2.  wait 20µs
		//delayMicroseconds(20);

		//3. place the device in core reset
      		address_write(0x02); // go to DEVCTL register
		data_write(0x04);    // write 0x04 to DEVCTL register

		//4. write 0x00 to EPCTL register
      		address_write(0xdf); // go to EPCTL register
		data_write(0x00);    // write 0x04 to DEVCTL register

		//5. write the first eprom address for reading to EPADDRH and EPADDRL
		unsigned char eprom_address = address; // eprom address i want
		address_write(EPADDRH); // eprom address to be read is written to EPADDRH register
		data_write(eprom_address);
		address_write(EPADDRL); // eprom address to be read is written to EPADDRL register
		data_write(eprom_address);

		//6. read a data byte from EPDAT (EPADDRH:L will increment by 1 after this read)
		address_write(EPDAT); // read result from epdat (0xbf)
		data_read();

		//7. (optional) check the error bit in register EPSTAT and abort memory read operation if necessary
		address_write(EPSTAT); // read EPSTAT (0xb7)
		data_read_epstat();
		address_write(EPDAT); // reset address read to EPDAT

		////8. if reading not finished return to step 6
		int iterations = 2048 - 1;
		for (int j = 0; j < iterations; j++) {
			data_read();
			//7. (optional) check the error bit in register EPSTAT and abort memory read operation if necessary
			address_write(EPSTAT); // read EPSTAT (0xb7)
			data_read_epstat();
			address_write(EPDAT); // reset address read to EPDAT
		}

		//9. remove read mode (1st step)
      		address_write(EPCTL); // go to EPCTL register
		data_write(0x40);    // write 0x40 to DEVCTL register

		//10. remove read mode (2nd step)
      		address_write(EPCTL); // go to EPCTL register
		data_write(0x00);    // write 0x00 to DEVCTL register

		//11. reset device
      		address_write(FPCTL); // go to DEVCTL register (=FPCTL)
		data_write(0x02);    // write 0x02 to DEVCTL register
      		//address_write(FPCTL); // go to DEVCTL register
		data_write(0x00);    // write 0x00 to DEVCTL register
		break;
	}

      /* AN127: READ AN EPROM BLOCK*/
      /* unfinished */
      case 'E':
      	{
        	unsigned char address = 0x800; // eprom adress i want to read
		int iterations = 256;

		//1. write 0x04 to FPCTL register (=DEVCTL)
      		address_write(FPCTL); // go to DEVCTL register
		data_write(0x04);    // write 0x04 to DEVCTL register

		//2. write 0x00 to EPCTL register
      		address_write(EPCTL);
		data_write(0x00);

		//3. write 0x58 to EPCTL register
      		address_write(EPCTL);
		data_write(0x58);

		cli();

		for (int k = 0; k < 24; k++)
		{

			//4. write the high byte of the address to EPADDRH
      			address_write(EPADDRH);
			data_write(address + 0xff);

			//5. write the low byte of the address to EPADDRL
      			address_write(EPADDRL);
			data_write(address);

			//6. perform an Address Write with a value of EPDAT
			address_write(EPDAT);

			//7.8. (9. optional) repeat steps 7 and 8 until all bytes are read
			for (int j = 0; j < iterations; j++) {
				//7. perform Address Read instructions until value returned is not 0x80
				//   and the EPROM is no longer busy
				address_read();

				//8. read the byte with data_read()
				data_read();

				////////(optional) check the error bit in register EPSTAT and abort memory read operation if necessary
				//address_write(EPSTAT); // read EPSTAT (0xb7)
				//data_read_epstat();
				//address_write(EPDAT); // reset address read to EPDAT
			}

			address += 0xff;
			delayMicroseconds(1000);
		}


		sei();

		// 10.11.12.13.14.
		address_write(0xdf);
		data_write(0x40);
		data_write(0x00);
		address_write(0x02);
		data_write(0x02);
		data_write(0x04);
		data_write(0x01);

		break;
	}

      case 'b':
      	{
        	unsigned char address = 0x0800; // eprom adress i want to read

		//1. write 0x04 to FPCTL register (=DEVCTL)
		write_direct(FPCTL, 0x04);

		//2. write 0x00 to EPCTL register
		write_direct(EPCTL, 0x00);

		//3. write 0x58 to EPCTL register
		write_direct(EPCTL, 0x58);

		//4. write the high byte of the address to EPADDRH
		unsigned char eprom_address = address; // eprom address i want
		address_write(EPADDRH); // eprom address to be read is written to EPADDRH register
		data_write(0x08ff); // + 256

		//5. write the low byte of the address to EPADDRL
		address_write(EPADDRL); // eprom address to be read is written to EPADDRL register
		data_write(eprom_address);

		//6. perform an Address Write with a value of EPDAT
		address_write(EPDAT);
		//data_read();

		//(optional) check the error bit in register EPSTAT and abort memory read operation if necessary
		//address_write(EPSTAT); // read EPSTAT (0xb7)
		//data_read_epstat();
		//address_write(EPDAT); // reset address read to EPDAT

		//7. perform Address Read instructions until value returned is not 0x80
		//   and the EPROM is no longer busy
		address_read();

		//8. read the byte with data_read()
		data_read();

		//9. repeat steps 7 and 8 until all bytes are read
		int iterations = 256 - 1;
		for (int j = 0; j < iterations; j++) {
			address_read();
			data_read();
			//address_write(EPSTAT); // read EPSTAT (0xb7)
			//data_read_epstat();
			//address_write(EPDAT); // reset address read to EPDAT
		}

		// 10.11.12.13.14.
      		address_write(EPCTL); // go to EPCTL register
		data_write(0x40);    // write 0x40 to DEVCTL register
		data_write(0x00);    // write 0x00 to DEVCTL register
      		address_write(FPCTL); // go to DEVCTL register (=FPCTL)
		data_write(0x02);    // write 0x02 to DEVCTL register
		data_write(0x04);    // write 0x02 to DEVCTL register
		data_write(0x01);    // write 0x00 to DEVCTL register
		break;
	}

    }
  }
}
