#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/socket.h>
#include  <netdb.h>
#include  <string.h>
#include  <unistd.h>
#include  <stdbool.h>

#define SERVADDR "localhost"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille du tampon de demande de connexion
#define MAXBUFFERLEN 1024
#define MAXHOSTLEN 64
#define MAXPORTLEN 6
#define MAXCMDELEN 100
#define MAXSERVLEN 100

int main(){

	struct addrinfo hints;          	// Contrôle la fonction getaddrinfo
	struct addrinfo *res, *resPtr;          // Contient le résultat de la fonction getaddrinfo
	struct sockaddr_storage myinfo; 	// Informations sur la connexion de RDV
	struct sockaddr_storage from;   	// Informations sur le client connecté
	socklen_t len;                  	// Variable utilisée pour stocker les longueurs des structures de socket

	
	//On déclare des fonctions afin de minimiser la taille du code
	int lectureSocket(int nomDescSock,char buffer[]);
	int creationSocket(char adresse[], char port[]);
	void transmission(int descSockOrigine, int descSockDestination, char buffer[]);

	// Publication de la socket au niveau du système
	// Assignation d'une adresse IP et un numéro de port
	// Mise à zéro de hints
	memset(&hints, 0, sizeof(hints));
	// Initailisation de hints
	hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
	hints.ai_socktype = SOCK_STREAM;  // TCP
	hints.ai_family = AF_INET;      // les adresses IPv4 et IPv6 seront présentées par la fonction getaddrinfo

	// Récupération des informations du serveur
	int ecode;                      	// Code retour des fonctions
	ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
	if (ecode) {fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));exit(1);
}
	//Création de la socket IPv4/TCP
	int descSockRDV;                	// Descripteur de socket de rendez-vous // initialisation
	descSockRDV = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (descSockRDV == -1) {perror("Erreur creation socket");exit(2);}

	// Publication de la socket
	ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
	if (ecode == -1) {perror("Erreur liaison de la socket de RDV");exit(3);}

	// Nous n'avons plus besoin de cette liste chainée addrinfo
	freeaddrinfo(res);

	//--------------------------------------------------------------------------------------AFFICHAGE INFO DE CONNEXION

	// Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
	char serverAddr[MAXHOSTLEN];    	// Adresse du serveur
	char serverPort[MAXPORTLEN];    	// Port du server
	len=sizeof(struct sockaddr_storage);
	ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
	if (ecode == -1){perror("SERVEUR: getsockname");exit(4);}

	ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr,MAXHOSTLEN, serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
	if (ecode != 0) {fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));exit(5);}

	//Affichage
	printf("L'adresse d'ecoute est: %s\n", serverAddr);
	printf("Le port d'ecoute est: %s\n", serverPort);

	//--------------------------------------------------------------------------------------GESTION CONNEXION CLIENT

	// Definition de la taille du tampon contenant les demandes de connexion
	ecode = listen(descSockRDV, LISTENLEN);
	if (ecode == -1) {perror("Erreur initialisation buffer d'écoute");exit(6);}
	len = sizeof(struct sockaddr_storage);

	// Lorsque demande de connexion, creation d'une socket de communication avec le client
	int descSockCOM;                	// Descripteur de socket de communication avec le client depuis le proxy
	descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len);
	if (descSockCOM == -1){perror("Erreur accept\n");exit(7);}

	//On demande à l'utilisateur de se connecter
	char buffer[MAXBUFFERLEN];      	// Tampon de communication entre le client et le serveur
	strcpy(buffer, "220 Utiliser ip@addrServ pour vous connecter\n");

	write(descSockCOM, buffer, strlen(buffer));

	//Tentative de connexion utilisateur
	lectureSocket(descSockCOM,buffer); // On attend LOGIN@r-info-onyx par exemple
	char userID[MAXCMDELEN];
	char serverName[MAXSERVLEN];
	sscanf(buffer, "%[^@]@%s", userID, serverName);

	//On se connecte au serveur ftp, on créer également un socket de communication
	// Echange de données avec le client connecté

	//Socket de communication avec le serveur
		
	int descSockServeur;			//Socket de communication avec le serveur depuis le proxy
	descSockServeur = creationSocket(serverName, "ftp");
	lectureSocket(descSockServeur,buffer);

		//On envoie les infos de connexions au serveur
		strcpy(buffer, userID);
		strcat(buffer, "\n");
		printf("%s\n", buffer);
		write(descSockServeur, buffer, strlen(buffer));

		lectureSocket(descSockServeur,buffer);

		//Demande du mdp: "331 Veuillez saisir le mot de passe" venant du serveur
		write(descSockCOM, buffer, strlen(buffer));

		lectureSocket(descSockCOM,buffer);

		write(descSockServeur, buffer, strlen(buffer));

		//On recoit la réponse de connexion du serveur ("230 Connecté" si tout se déroule comme prévu)
		lectureSocket(descSockServeur,buffer);

		write(descSockCOM, buffer, strlen(buffer));

		int ac1, ac2, ac3, ac4, pc1, pc2;
		char adrClient[50], portClient[50];
		char adrServeur[50], portServeur[50];

		lectureSocket(descSockCOM,buffer);

	//--------------------------------------------------------------------------------------MISE EN PLACE GESTION ENTREE COMMANDE UTILISATEUR

	//Tant que la commande QUIT n'est pas entrée, on lit les commande client et on transmet.
		while(strncmp(buffer,"QUIT",4)) {

			if(strncmp(buffer, "PORT", 4) == 0){

				sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &ac1,&ac2,&ac3,&ac4,&pc1,&pc2);
				sprintf(adrClient, "%d.%d.%d.%d", ac1,ac2,ac3,ac4);
				sprintf(portClient, "%d", pc1*256+pc2);

				//On répond le code 200
				strcpy(buffer, "200 PORT correct\n");
				write(descSockCOM, buffer, strlen(buffer));

				//Récupération des ports serveur
				strcpy(buffer, "PASV\n");
				write(descSockServeur, buffer, strlen(buffer));

				lectureSocket(descSockServeur,buffer);

				//Lecture port
				sscanf(buffer, "%*[^(](%d,%d,%d,%d,%d,%d", &ac1,&ac2,&ac3,&ac4,&pc1,&pc2);
				sprintf(adrServeur, "%d.%d.%d.%d", ac1,ac2,ac3,ac4);
				sprintf(portServeur, "%d", pc1*256+pc2);

				//Lire LIST et l'envoyer au serveur puis créer le nouveau socket de transmission des données
				lectureSocket(descSockCOM,buffer);
				write(descSockServeur, buffer, strlen(buffer)); //envoi de la commande au serveur

				//Création d'un nouveau socket pour les données venant du serveur
				int descSockServData;
				descSockServData=creationSocket(adrServeur, portServeur);
				
				// On recoit du serveur le code 150 que l'on transmet au client
				lectureSocket(descSockServeur,buffer);
				write(descSockCOM, buffer, strlen(buffer));

				//On crée le socket de transmissions des données au client descSockClientData
				int descSockClientData;			//Socket de communication avec le serveur depuis le
				descSockClientData=creationSocket(adrClient, portClient);
						
				//On initialise le buffer de transmission
				//ecode=lectureSocket(descSockServData,buffer);

				ecode = read(descSockServData, buffer, MAXBUFFERLEN);
				if (ecode == -1) {perror("Problème de lecture\n"); exit(99);}
				buffer[ecode] = '\0';
				//On transmet les données du serveur au client
				while(ecode != 0){
					ecode = write(descSockClientData, buffer, strlen(buffer));				
					lectureSocket(descSockServData,buffer);
				}
				close(descSockClientData);
				close(descSockServData);

				lectureSocket(descSockServeur,buffer);

			}else{
				//Traiter les commandes sans port
				write(descSockServeur, buffer, strlen(buffer));
				lectureSocket(descSockServeur,buffer);

			} //Fin SI
			//Réponse du serveur quoiqu'il arrive
			write(descSockCOM, buffer, strlen(buffer));
			lectureSocket(descSockCOM,buffer);
		} //Fin WHILE
	//Fermeture de la connexion
	close(descSockCOM);
	close(descSockRDV);
	close(descSockServeur);

	
}

//Occurences : 12
int lectureSocket(int nomDescSock, char buffer[]){ 
	int ecode;
	ecode = read(nomDescSock, buffer, MAXBUFFERLEN);
	if (ecode == -1) {perror("Problème de lecture\n"); exit(99);}
	buffer[ecode] = '\0';
}
//Occurences : 3
int creationSocket(char adresse[], char port[]){ 
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;  
	hints.ai_family = AF_INET;
	int ecode;
	int descRetour; 
	struct addrinfo *res, *resPtr;
	ecode = getaddrinfo(adresse,port,&hints,&res);
	if (ecode){fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));exit(8);}
	int isConnected=0;

	resPtr = res;
	while(isConnected==0 && resPtr!=NULL){
	//Création de la socket IPv4/TCP
		descRetour = socket(resPtr->ai_family, resPtr->ai_socktype, resPtr->ai_protocol);
		if (descRetour == -1) {perror("Erreur creation socket client");exit(9);}

	  	//Connexion au serveur
		ecode = connect(descRetour, resPtr->ai_addr, resPtr->ai_addrlen);
		if (ecode == -1) {resPtr = resPtr->ai_next;close(descRetour);}
		else { isConnected = 1;}
	}

	freeaddrinfo(res);
	if (isConnected==0){perror("Connexion impossible !");exit(10);}
	return descRetour;
}
void transmission(int descSockOrigine, int descSockDestination, char buffer[]){
	lectureSocket(descSockOrigine, buffer);
	write(descSockDestination, buffer, strlen(buffer));
}	














