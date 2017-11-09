#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>
#include <fcntl.h>

#define MAX_ARG 8
#define MAX_LENGTH 32

#define MAX_LINE 256
#define END_CMD "goodbye"
#define END_MESSAGE "Bye bye!\n"
#define PROMPT "20153204_gy> "
#define IS_WHITE_SPACE(x) ((x)==' ' || (x)=='\t')
#define IS_PIPE(x) ((x)=='|')
#define IS_OUT_FILE_REDIRECTION(x) ((x)=='>')
#define IS_IN_FILE_REDIRECTION(x) ((x)=='<')
#define IS_EOS(x) ((x) == '\n' || (x) == '\0')
#define PREFIX "/bin"
#define NOT_COMMAND ": command not found\n"
#define PIPE_ERROR "Pipe error"

int instruction_start(char buffer[MAX_LENGTH]);

int main()
{
	int p, s;
	int ins_p, ins_s;

	char buffer[MAX_LINE];
	char parse[MAX_ARG][MAX_LENGTH];
	char *arg[MAX_ARG];
	int i, j, cursor, arg_num;
	int usePipe;
	int fd[2];


	while (1)
	{
		if (p = fork())  // 자식 프로세스 fork
		{
			waitpid(p, &s, 0);   //자식 프로세스를 부모가 끝날 때까지 wait
								 /*return 값이 1일 경우 "goodbye" 이므로, "Bye Bye"를 출력*/
			if (WEXITSTATUS(s) == 1)
			{
				write(1, END_MESSAGE, strlen(END_MESSAGE));
				break;
			}
			/*return 값이 2일 경우, 명령어가 없으므로 ": command not found" 출력*/
			else if (WEXITSTATUS(s) == 2)
			{
				write(1, NOT_COMMAND, strlen(NOT_COMMAND));
			}
			/*return 값이 3일 경우, pipe(fd)<0 에러이므로, "Pipe error" 출력*/
			else if (WEXITSTATUS(s) == 3)
			{
				write(1, PIPE_ERROR, strlen(PIPE_ERROR));
			}
		}
		else //자식 프로세스일 경우
		{
			write(1, PROMPT, strlen(PROMPT));
			read(0, buffer, MAX_LINE);

			/*pipe 에러 시 3을 리턴*/
			if (pipe(fd) < 0) return 3;

			i = j = cursor = 0;

			/*최초 공백을 제거한다.*/
			while (IS_WHITE_SPACE(buffer[cursor])) cursor++;

			/*문장이 끝날 때까지 반복, pipe가 입력된 경우와 아닌 경우로 나눔*/
			while (!IS_EOS(buffer[cursor]))
			{
				/*PIPE('|')명령이 입력될 경우, 공백을 제거하고 배열 포인터 이동*/
				if (IS_PIPE(buffer[cursor]))
				{
					cursor++;    // '|' 문자는 제거

					parse[i][j] = '\0';

					/*공백 제거*/
					while (!IS_EOS(buffer[cursor]) && IS_WHITE_SPACE(buffer[cursor])) cursor++;

					/*문장이 끝이 나지 않는다면 다음 배열로 이동*/
					if (!IS_EOS(buffer[cursor]))
					{
						i++;
						j = 0;
					}
				}
				/*pipe 명령('|')이 입력되지 않을 경우, 입력내용 buffer에 저장*/
				else parse[i][j++] = buffer[cursor++];
			}
			parse[i++][j] = '\0';

			arg_num = i;
			usePipe = (arg_num > 1);

			/*배열에 저장된 명령어 개수 만큼 자식 프로세스를 호출*/
			for (i = 0; i < arg_num; i++) {


				if (ins_p = fork())    //자식 프로세스 호출
				{
					waitpid(ins_p, &ins_s, 0);    //하나의 명령어가 처리될 때까지 자식 프로세스 wait
												  /*shell을 끝낼 경우 return 1*/
					if (WEXITSTATUS(ins_s) == 1)
					{
						return 1;
					}
					/*command가 정의되어 있지 않은 경우 return 2*/
					else if (WEXITSTATUS(ins_s) == 2)
					{
						return 2;
					}
				}
				else  //자식 프로세스
				{
					if (i > 0) dup2(fd[0], STDIN_FILENO);   //최초 명령어를 제외하고 stdin을 파이프로 연결 (최초 명령어는 화면으로 stdin)
					if (i < arg_num - 1) dup2(fd[1], STDOUT_FILENO); //마지막 명령어를 제외하고 stdout을 파이프로 연결 (마지막 명령어는 화면으로 stdout)
					return instruction_start(parse[i]); //두 번째 자식 프로세스 이후에는 instruction_start 함수 호출
				}
			}
			return 0;
		}
	}
}

int instruction_start(char buffer[MAX_LENGTH])
{
	char outFilePath[MAX_LENGTH];
	char errFilePath[MAX_LENGTH];
	char inFilePath[MAX_LENGTH];
	char *pathPointer;
	char parse[MAX_ARG][MAX_LENGTH];
	char path[MAX_LENGTH];
	char *arg[MAX_ARG];

	int useInFile;
	int useOutFile;
	int useErrFile;

	int i, j, cursor, arg_num;

	useOutFile = useErrFile = 0;
	i = j = cursor = 0;

	/*명령어와 명령어 사이의 공백 제거*/
	while (IS_WHITE_SPACE(buffer[cursor])) cursor++;

	/*입력 커맨드가 없으면 0을 리턴*/
	if (IS_EOS(buffer[cursor])) return 0;
	while (!IS_EOS(buffer[cursor]))
	{
		/* 리다이렉션 '>' 혹은 '<'가 입력될 경우*/
		if (IS_OUT_FILE_REDIRECTION(buffer[cursor]) || IS_IN_FILE_REDIRECTION(buffer[cursor]))
		{
			/*2> 리다이렉션 일 경우*/
			if (cursor > 1 && buffer[cursor - 2] == ' ' && buffer[cursor - 1] == '2')
			{
				i--;
				j = strlen(parse[i]);
				useErrFile = 1;      //ErrFile을 쓸 것이라고 명시
				pathPointer = errFilePath; //pathPointer를 errFilePath로 설정
			}
			/*> 리다이렉션일 경우*/
			else if (IS_OUT_FILE_REDIRECTION(buffer[cursor]))
			{
				useOutFile = 1;  //outFile을 쓸 것이라고 명시  
				pathPointer = outFilePath;   //pathPointer를 outFilePath로 설정
			}
			/*< 리다이렉션일 경우*/
			else {
				useInFile = 1;  //infile을 쓸 것이라고 명시
				pathPointer = inFilePath;   //pathPointer를 inFilepATH로 설정
			}

			cursor++;

			/*명령어와 명령어 사이의 공백 제거*/
			while (IS_WHITE_SPACE(buffer[cursor])) cursor++;

			/*PathPointer에 버퍼의 내용을 옮김*/
			while (!IS_EOS(buffer[cursor]) && !IS_WHITE_SPACE(buffer[cursor]))
			{
				*pathPointer = buffer[cursor];
				pathPointer++;
				cursor++;
			}
			*pathPointer = '\0';

		}
		/*공백이 발생한 경우*/
		else if (IS_WHITE_SPACE(buffer[cursor]))
		{
			parse[i][j] = '\0';
			while (!IS_EOS(buffer[cursor]) && IS_WHITE_SPACE(buffer[cursor])) cursor++;

			if (!IS_EOS(buffer[cursor]) && !IS_OUT_FILE_REDIRECTION(buffer[cursor]) && !IS_IN_FILE_REDIRECTION(buffer[cursor]))
			{
				i++;  //배열 이동
				j = 0;
			}
		}
		else parse[i][j++] = buffer[cursor++];  //명령어가 들어올 경우 버퍼에 저장
	}
	parse[i++][j] = '\0';
	arg_num = i;

	/*execv가 NULL(0)으로 끝을 판단하도록 0을 입력*/
	for (i = 0; i < arg_num; i++) arg[i] = parse[i];
	arg[i] = (char *)0;

	/*goodbye가 입력된 경우 1을 리턴*/
	if (!strcmp(arg[0], END_CMD)) return 1;

	/*2> 리다이렉션을 사용한 경우*/
	if (useErrFile == 1)
	{
		/*stderr을 닫고 dup으로 파이프 연결*/
		int fd = open(errFilePath, O_CREAT | O_WRONLY, 0644);
		close(2);
		dup(fd);
	}
	/*> 리다이렉션을 사용한 경우*/
	if (useOutFile == 1)
	{
		/*stdout을 닫고 dup으로 파이프 연결*/
		int fd = open(outFilePath, O_CREAT | O_WRONLY, 0644);
		close(1);
		dup(fd);
	}
	/*< 리다이렉션을 사용한 경우*/
	if (useInFile == 1)
	{
		/*stdin을 닫고 dup으로 파이프 연결*/
		int fd = open(inFilePath, O_RDONLY);
		close(0);
		dup(fd);
	}

	/*path 생성*/
	strcpy(path, PREFIX);
	strcat(path, "/");
	strcat(path, arg[0]);

	/*exec 호출*/
	execv(path, arg);

	/*exec 실패 시 command 출력 후 2를 리턴*/
	write(STDOUT_FILENO, arg[0], strlen(arg[0]));
	return 2;
}
