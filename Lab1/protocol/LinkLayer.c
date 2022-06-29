#include "linklayer.h"
#include "linkv2.h"


int fd;													
unsigned char C_FIELD_aux = C00;
int COMPLETO;
int LENGTH;
int tentativas = 0;
unsigned char * CopiedPacket;
unsigned char SET[5];
unsigned char UA[5];
unsigned char DISC[5];
unsigned char generic[5];
linkLayer new;
Statistics stats;

/* Send Packet
 * envia todo o tipo de tramas e é utilizada pelo transmissor e pelo recetor.
 */

void enviaTrama()
{
		COMPLETO = FALSE;
		int time = 0;

		printf("time out fornecido %d\n", new.timeOut);

		if(tentativas > 0)
			(stats.timeouts)++;

		if( new.role == TRANSMITTER )
		{
			if(tentativas == new.numTries)
			{
				printf("TIMEOUT: Failed 3 times, exiting...\n");
				exit(-1);
			}
			printf("Attempt number %d\n", tentativas+1);

			int BytesSent = 0;

			while( BytesSent != LENGTH)
			{
				BytesSent = write(fd, CopiedPacket, LENGTH);
				stats.bytes_sent+=LENGTH;
			}

			tentativas++;

			if(!COMPLETO)
			{
				alarm(new.timeOut);
			}
		}

			else if( new.role == RECEIVER )
			{
				int BytesSent = 0;

				while( BytesSent != 5 )
				{
					BytesSent = write(fd, CopiedPacket, 5);
				}
			}

			else
			{
				printf("ERROR\n");
				exit(-1);
			}

}



void trama_ready(unsigned char * array, unsigned char C)
{
	array[0] = FLAG;
	array[1] = A;
	array[2] = C;
	array[3] = array[1]^array[2];
	array[4] = FLAG;
}


/* recebeTrama
 * receção de tramas de controlo
 */

unsigned char recebeTrama(int fd)
{
	unsigned char car, res, Cverif;
	int state=0;

	while( state != 5 )
	{
    res = read(fd,&car,1);
    if( res < 0)
    {
      printf("Read falhou.\n");
      return FALSE;
  	}

		switch(state)
		{
			case 0: //expecting flag
				if(car == FLAG)
					state = 1;
				break;

			case 1: //expecting A
				if(car == A)
					state = 2;

				else if(car != FLAG)
				{
					state = 0;
				}

				break;

			case 2: //expecting Cverif
				Cverif = car;
				if(car == C_UA || car == C_DISC || car == C_SET)
					state = 3;

				else if(car != FLAG)
				{
					state = 1;
				}

				else
					state = 0;

				break;

			case 3: //expecting BCC
				if(car == (A^Cverif))
					state = 4;

				else
				{
					state = 0;
				}

				break;

			case 4: //expecting FLAG
				if(car == FLAG)
					state = 5;

				else
				{
					state = 0;
				}

				break;
		}
	}

	COMPLETO = TRUE;
	alarm(0);
	printf("Success!\n");
	tentativas = 0;

	return Cverif;
}

/* llopen
 * 
 */


int llopen(linkLayer connectionParameters)
{
	signal(SIGALRM, enviaTrama);	

	printf("%s\n", connectionParameters.serialPort);
	fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

	if( fd < 0 )
	{
		printf("Error at opening connection!\n");
		return NOT_DEFINED;
	}

	if ( tcgetattr(fd,&oldtio) == -1) { 
      perror("tcgetattr");
      exit(-1);
	}

	bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    //set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  	newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

		CopiedPacket = (unsigned char *) malloc(sizeof(unsigned char));

	new = connectionParameters;

	stats.retransmissions = 0;
	stats.timeouts = 0;
	stats.I_Frame_Received = 0;

	switch(connectionParameters.role)
	{
		case TRANSMITTER:
			trama_ready(SET, C_SET);

			copyToPacketToSend(SET, 5);

			enviaTrama();
			printf("C_SET enviado\n");

			if(recebeTrama(fd) == C_UA)
				printf("C_UA recebido\n");
			break;

		case RECEIVER:
			if(recebeTrama(fd) == C_SET){
				printf("C_SET recebido\n");
				trama_ready(UA, C_UA);

				copyToPacketToSend(UA, 5);

				enviaTrama();
				printf("C_UA enviado\n");
			break;
			}

	}

	printf("ligação estabelecida\n");

	return TRUE;
}

/* llclose
 * 
 */


int llclose(int showStatistics)
{
		switch(new.role)
		{
			case TRANSMITTER:
				trama_ready(generic, C_DISC);
				copyToPacketToSend(generic, 5);
				enviaTrama();
				printf("C_DISC sent\n");

				if(recebeTrama(fd) == C_DISC)
					printf("C_DISC received\n");

					trama_ready(UA, C_UA);
					copyToPacketToSend(UA, 5);
					enviaTrama();

					printf("WRITE ACABOU\n");

					if( tcsetattr(fd, TCSANOW, &oldtio) == -1)
					{
						perror("tcsetattr");
						exit(-1);
					}
				break;

			case RECEIVER:
				if(recebeTrama(fd) == C_DISC){
					printf("C_DISC received\n");

					trama_ready(generic, C_DISC);
					copyToPacketToSend(generic, 5);
					enviaTrama();
					printf("C_DISC sent\n");

					if(recebeTrama(fd) == C_UA)
						printf("C_UAC received\n");

						printf("READ ACABOU\n");

						if( tcsetattr(fd, TCSANOW, &oldtio) == -1)
						{
							perror("tcsetattr");
							exit(-1);
						}
				break;
				}

		}

		printf("Estatística:\n");
		printf("How many retransmission = %d\n", stats.retransmissions);
		printf("How many timeouts = %d\n", stats.timeouts);
		printf("How many  bytes were sent = %d\n", stats.bytes_sent);
		printf("Frames received = %d\n", stats.I_Frame_Received);

		return 1;
}

/* llwrite
 * 
 */


int llwrite(char * buf, int size){

	int sizeNewMsg = size + 6;
	int BytesSent = 0;
	int retransmission = -1; 	

	unsigned char *newMsg = (unsigned char *)malloc(sizeof(unsigned char) * (size+6));

	unsigned char BCC2;
	int sizeBCC2 = 1;
	BCC2 = BCC2_ver(buf, size);


	newMsg[0] = FLAG;
	newMsg[1] = A;
	newMsg[2] = C_FIELD_aux;
	newMsg[3] = newMsg[1]^newMsg[2];

	sizeNewMsg = stuffing(buf, newMsg, size, sizeNewMsg);

	newMsg[sizeNewMsg-2] = BCC2;
	newMsg[sizeNewMsg-1] = FLAG;

	unsigned char readChar;

	copyToPacketToSend(newMsg, sizeNewMsg);

	do
	{
			retransmission++;

			if( retransmission > MAX_RETRANSMISSIONS_DEFAULT )
			{
				stats.retransmissions = MAX_RETRANSMISSIONS_DEFAULT;
					printf("Atingiu limite de retransmissões\n");
					return -1;
			}

			enviaTrama();
			readChar = getCmdExpectingTwo(correctREJ(newMsg[2]), correctRR(newMsg[2]));

	} while( readChar == correctREJ(newMsg[2]) );

	(stats.retransmissions)+=retransmission;

	complementC();

	free(newMsg);

	return sizeNewMsg;

}

/* llread
 * 
 */


int llread(char *packet)
{

	int size=0, n, res;
	unsigned char *auxbuff = (unsigned char *) malloc(size*sizeof(char));
	unsigned char ctrl_aux;
	unsigned char c;
	int state = 0;
	int i;

  while (state != 5)
  {
		res = read(fd, &c, 1);

    if(res < 0)
		{
			perror("llread");
			return FALSE;
		}

    switch (state)
    {
    //flag
    case 0:
      if (c == FLAG)
        state = 1;
      break;
    // A
    case 1:
      if (c == A)
        state = 2;
      else
      {
        if (c == FLAG)
          state = 1;
        else
          state = 0;
      }
      break;
    // C
    case 2:

      if (c == C00 || c == C10)
      {
				ctrl_aux= c;
        state = 3;
      }

      else
      {
        if (c == FLAG)
          state = 1;
        else
          state = 0;
      }
      break;
    //BCC
    case 3:
      if (c == (A ^ ctrl_aux))
			{
				  state = 4;
					i = 0;
			}
      else
        state = 0;									
      break;
    //segunda FLAG
    case 4:
		++size;
		auxbuff = (unsigned char *) realloc(auxbuff, size*sizeof(unsigned char));
		auxbuff[i] = c;	

		if(c == FLAG){

			n = remove_extra(auxbuff, ctrl_aux, size);

			if(n != -1){
					for (i = 0; i < n; i++) {
						packet[i] = auxbuff[i];
					}
					state = 5;
			}

			else{
				printf("pacote corrompido, REJ enviado\n");
				return 0;
			}

		}
		i++;
		break;
	}
  }
	return n;
}

/* destuffing
 * 
 */


int destuffing(unsigned char *vet, int size){
	int n = 0;
	for(int i = 0; i < size; i++){
		if(vet[i] == ESCAPE){
			if(vet[i+1] == ESCAPE_FLAG){						
				vet[i] = FLAG;
				for(int j = i+1; j<=size ; j++){
					vet[j]=vet[j+1];
				}
				n++;
			}
			if(vet[i+1] == ESCAPE_ESCAPE){					
				for(int j = i+1; j<=size ; j++){
					vet[j]=vet[j+1];
				}
				n++;
			}
		}
	}
	return n;}


/* remove extra
 * 
 */

int remove_extra(unsigned char *buffer, char ctrl_aux, int size)
{
	int reducedSize = 0;
	int flags;

	unsigned char bcc2vet = buffer[size-2];

	flags = destuffing(buffer, size-2);						

	reducedSize = size - flags - 2;

	buffer = (unsigned char *) realloc(buffer, reducedSize);

		unsigned char bcc2cal = BCC2_ver(buffer, reducedSize);

	if( bcc2cal == bcc2vet){

		if(ctrl_aux == C00)
		{
			C_FIELD_aux = RR1;
			trama_ready(generic, C_FIELD_aux);
			copyToPacketToSend(generic, 5);
			enviaTrama();
		}
		else
		{		//C == C10
			C_FIELD_aux = RR0;
			trama_ready(generic, C_FIELD_aux);

			copyToPacketToSend(generic, 5);
			enviaTrama();
		}
		(stats.I_Frame_Received)++;
		return reducedSize;
	}

	else
	{
		if(ctrl_aux == C00){
			C_FIELD_aux = REJ1;								
			trama_ready(generic, C_FIELD_aux);
			copyToPacketToSend(generic, 5);
			enviaTrama();
		}
		else{	
			C_FIELD_aux = REJ0;
			trama_ready(generic, C_FIELD_aux);
			copyToPacketToSend(generic, 5);
			enviaTrama();
		}

		return -1;
	}
}


/* calculo de BCC2
 * 
 */
unsigned char BCC2_ver(unsigned char *arr, int size)
{
    unsigned char result = arr[0];

    for(int i = 0; i < size; i++)
        result ^= arr[i];

    return result;
}


/* stuffing
 * 
 */
int stuffing(char * buf, unsigned char * Newmsg, int size, int sizeBuf)
{
	int aux=4;

	for(int i = 0; i < size; i++)
	{
		if( buf[i] == FLAG )
		{
			sizeBuf++;
			Newmsg = (unsigned char *)realloc(Newmsg, sizeBuf);
			Newmsg[aux] = ESCAPE;
			Newmsg[aux+1] = ESCAPE_FLAG;

			aux = aux + 2;
		}

		else if( buf[i] == ESCAPE )
		{
			sizeBuf++;
			Newmsg = (unsigned char *)realloc(Newmsg, sizeBuf);
			Newmsg[aux] = ESCAPE;
			Newmsg[aux+1] = ESCAPE_ESCAPE;

			aux = aux + 2;
		}

		else
		{
			Newmsg[aux] = buf[i];
			aux++;
		}

	}

	return sizeBuf;
}

/* O REJ que é esperado
 * 
 */
unsigned char correctREJ(unsigned char var)	
{
  if( var == C00 )
    return REJ1;

	else
    return REJ0;
}

/* O RR que é esperado
 * 
 */
unsigned char correctRR(unsigned char var)
{
  if(var == C00)
    return RR1;

	else
    return RR0;
}


/* Complemento (0/1)
 * 
 */
void complementC()
{
  if(C_FIELD_aux == C00)
    C_FIELD_aux = C10;

  else
    C_FIELD_aux = C00;
}


unsigned char getCmdExpectingTwo(unsigned char expecting1, unsigned char expecting2)
{
	unsigned char readChar, res;
	unsigned char matchExpected = 0;
	unsigned char packet_A, packet_C;

	int state = 0;

	while (state != 5)
	{       /* loop for input */

			res = read(fd, &readChar, 1);

  		if(res < 0)
			{
	      printf("Read falhou.\n");
	      exit(-1);
	  	}

  		switch (state)
			{
				case 0:
  				if (readChar == FLAG)
  					state = 1; 
  				break;

  			case 1:
  				packet_A = readChar; 

  				if(readChar == A)
  					state = 2;

					else if(readChar != FLAG)
  					state = 0;
  				break;

  			case 2:
  				packet_C = readChar; 

					if(readChar == expecting1 || readChar == expecting2)
					{
						matchExpected = readChar;
  					state=3;
					}

					else if(readChar != FLAG)
						state = 1;

						else
							state=0;
  					break;

  			case 3:
  				if(readChar == (packet_A ^ packet_C)) //expecting bcc1
  					state=4;

  				else
  					state=0;
  				break;

  			case 4:
  				if(readChar == FLAG)
  					state = 5;

  				else
  					state=0;
  				break;
  		}
    }

    COMPLETO = TRUE;
		alarm(0);
		tentativas = 0;

	return matchExpected;
}

/* Pacote copiado
 * 
 */
void copyToPacketToSend(unsigned char * sourcePacket, int length)
{
	LENGTH = length;
	CopiedPacket = (unsigned char *) realloc(CopiedPacket,sizeof(unsigned char) * LENGTH);

  for(int i = 0; i < length; i++){
    CopiedPacket[i] = sourcePacket[i];
  }
}
