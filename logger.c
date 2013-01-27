#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sqlite3.h>

#define PORT "55555"
#define MAXBUFLEN 512

int main(int argc, const char *argv[])
{
	struct addrinfo hints, *res;
	struct sockaddr_storage recv_addr;
	char buf[MAXBUFLEN];
	int sockfd, recv_addr_len;
	char *cursor;

	char configname[32], mapname[32];
	int alivesurvs, survcompletion[4], survhealth[4], bossflow[2];

	sqlite3 *db;
	int rc;
	char query[256];
	sqlite3_stmt *ppStmt;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s database\n", argv[0]);
		exit(1);
	}
	
	rc = sqlite3_open(argv[1], &db);
	if (rc)
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, PORT, &hints, &res);

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	bind(sockfd, res->ai_addr, res->ai_addrlen);

	for (;;)
	{
		memset(buf, 0, sizeof(buf));
		cursor = buf;
		recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr *)&recv_addr,
				&recv_addr_len);

		strcpy(configname, cursor);
		cursor += 1 + strlen(cursor);
		strcpy(mapname, cursor);
		cursor += 1 + strlen(cursor);
		memcpy(&alivesurvs, cursor, 4);
		cursor += 4;
		memcpy(survcompletion, cursor, 16);
		cursor += 16;
		memcpy(survhealth, cursor, 16);
		cursor += 16;
		memcpy(bossflow, cursor, 8);
		cursor += 16;

		printf("%s,%s,", configname, mapname);
		printf("%d,%d,%d,%d,%d,", alivesurvs, survcompletion[0], survcompletion[1], survcompletion[2], survcompletion[3]);
		printf("%d,%d,%d,%d,", survhealth[0], survhealth[1], survhealth[2], survhealth[3]);
		printf("%d,%d\n", bossflow[0], bossflow[1]);

		sprintf(query, "INSERT INTO log VALUES(\"%s\", \"%s\", %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)", configname, mapname, alivesurvs, survcompletion[0], survcompletion[1], survcompletion[2], survcompletion[3], survhealth[0], survhealth[1], survhealth[2], survhealth[3], bossflow[0], bossflow[1]);

		sqlite3_prepare_v2(db, query, sizeof(query), &ppStmt, NULL);
		sqlite3_step(ppStmt);
		sqlite3_finalize(ppStmt);
	}

	sqlite3_close(db);
	freeaddrinfo(res);
	close(sockfd);
	exit(0);
}