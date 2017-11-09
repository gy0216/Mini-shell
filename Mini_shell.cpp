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
		if (p = fork())  // �ڽ� ���μ��� fork
		{
			waitpid(p, &s, 0);   //�ڽ� ���μ����� �θ� ���� ������ wait
								 /*return ���� 1�� ��� "goodbye" �̹Ƿ�, "Bye Bye"�� ���*/
			if (WEXITSTATUS(s) == 1)
			{
				write(1, END_MESSAGE, strlen(END_MESSAGE));
				break;
			}
			/*return ���� 2�� ���, ��ɾ �����Ƿ� ": command not found" ���*/
			else if (WEXITSTATUS(s) == 2)
			{
				write(1, NOT_COMMAND, strlen(NOT_COMMAND));
			}
			/*return ���� 3�� ���, pipe(fd)<0 �����̹Ƿ�, "Pipe error" ���*/
			else if (WEXITSTATUS(s) == 3)
			{
				write(1, PIPE_ERROR, strlen(PIPE_ERROR));
			}
		}
		else //�ڽ� ���μ����� ���
		{
			write(1, PROMPT, strlen(PROMPT));
			read(0, buffer, MAX_LINE);

			/*pipe ���� �� 3�� ����*/
			if (pipe(fd) < 0) return 3;

			i = j = cursor = 0;

			/*���� ������ �����Ѵ�.*/
			while (IS_WHITE_SPACE(buffer[cursor])) cursor++;

			/*������ ���� ������ �ݺ�, pipe�� �Էµ� ���� �ƴ� ���� ����*/
			while (!IS_EOS(buffer[cursor]))
			{
				/*PIPE('|')����� �Էµ� ���, ������ �����ϰ� �迭 ������ �̵�*/
				if (IS_PIPE(buffer[cursor]))
				{
					cursor++;    // '|' ���ڴ� ����

					parse[i][j] = '\0';

					/*���� ����*/
					while (!IS_EOS(buffer[cursor]) && IS_WHITE_SPACE(buffer[cursor])) cursor++;

					/*������ ���� ���� �ʴ´ٸ� ���� �迭�� �̵�*/
					if (!IS_EOS(buffer[cursor]))
					{
						i++;
						j = 0;
					}
				}
				/*pipe ���('|')�� �Էµ��� ���� ���, �Է³��� buffer�� ����*/
				else parse[i][j++] = buffer[cursor++];
			}
			parse[i++][j] = '\0';

			arg_num = i;
			usePipe = (arg_num > 1);

			/*�迭�� ����� ��ɾ� ���� ��ŭ �ڽ� ���μ����� ȣ��*/
			for (i = 0; i < arg_num; i++) {


				if (ins_p = fork())    //�ڽ� ���μ��� ȣ��
				{
					waitpid(ins_p, &ins_s, 0);    //�ϳ��� ��ɾ ó���� ������ �ڽ� ���μ��� wait
												  /*shell�� ���� ��� return 1*/
					if (WEXITSTATUS(ins_s) == 1)
					{
						return 1;
					}
					/*command�� ���ǵǾ� ���� ���� ��� return 2*/
					else if (WEXITSTATUS(ins_s) == 2)
					{
						return 2;
					}
				}
				else  //�ڽ� ���μ���
				{
					if (i > 0) dup2(fd[0], STDIN_FILENO);   //���� ��ɾ �����ϰ� stdin�� �������� ���� (���� ��ɾ�� ȭ������ stdin)
					if (i < arg_num - 1) dup2(fd[1], STDOUT_FILENO); //������ ��ɾ �����ϰ� stdout�� �������� ���� (������ ��ɾ�� ȭ������ stdout)
					return instruction_start(parse[i]); //�� ��° �ڽ� ���μ��� ���Ŀ��� instruction_start �Լ� ȣ��
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

	/*��ɾ�� ��ɾ� ������ ���� ����*/
	while (IS_WHITE_SPACE(buffer[cursor])) cursor++;

	/*�Է� Ŀ�ǵ尡 ������ 0�� ����*/
	if (IS_EOS(buffer[cursor])) return 0;
	while (!IS_EOS(buffer[cursor]))
	{
		/* �����̷��� '>' Ȥ�� '<'�� �Էµ� ���*/
		if (IS_OUT_FILE_REDIRECTION(buffer[cursor]) || IS_IN_FILE_REDIRECTION(buffer[cursor]))
		{
			/*2> �����̷��� �� ���*/
			if (cursor > 1 && buffer[cursor - 2] == ' ' && buffer[cursor - 1] == '2')
			{
				i--;
				j = strlen(parse[i]);
				useErrFile = 1;      //ErrFile�� �� ���̶�� ���
				pathPointer = errFilePath; //pathPointer�� errFilePath�� ����
			}
			/*> �����̷����� ���*/
			else if (IS_OUT_FILE_REDIRECTION(buffer[cursor]))
			{
				useOutFile = 1;  //outFile�� �� ���̶�� ���  
				pathPointer = outFilePath;   //pathPointer�� outFilePath�� ����
			}
			/*< �����̷����� ���*/
			else {
				useInFile = 1;  //infile�� �� ���̶�� ���
				pathPointer = inFilePath;   //pathPointer�� inFilepATH�� ����
			}

			cursor++;

			/*��ɾ�� ��ɾ� ������ ���� ����*/
			while (IS_WHITE_SPACE(buffer[cursor])) cursor++;

			/*PathPointer�� ������ ������ �ű�*/
			while (!IS_EOS(buffer[cursor]) && !IS_WHITE_SPACE(buffer[cursor]))
			{
				*pathPointer = buffer[cursor];
				pathPointer++;
				cursor++;
			}
			*pathPointer = '\0';

		}
		/*������ �߻��� ���*/
		else if (IS_WHITE_SPACE(buffer[cursor]))
		{
			parse[i][j] = '\0';
			while (!IS_EOS(buffer[cursor]) && IS_WHITE_SPACE(buffer[cursor])) cursor++;

			if (!IS_EOS(buffer[cursor]) && !IS_OUT_FILE_REDIRECTION(buffer[cursor]) && !IS_IN_FILE_REDIRECTION(buffer[cursor]))
			{
				i++;  //�迭 �̵�
				j = 0;
			}
		}
		else parse[i][j++] = buffer[cursor++];  //��ɾ ���� ��� ���ۿ� ����
	}
	parse[i++][j] = '\0';
	arg_num = i;

	/*execv�� NULL(0)���� ���� �Ǵ��ϵ��� 0�� �Է�*/
	for (i = 0; i < arg_num; i++) arg[i] = parse[i];
	arg[i] = (char *)0;

	/*goodbye�� �Էµ� ��� 1�� ����*/
	if (!strcmp(arg[0], END_CMD)) return 1;

	/*2> �����̷����� ����� ���*/
	if (useErrFile == 1)
	{
		/*stderr�� �ݰ� dup���� ������ ����*/
		int fd = open(errFilePath, O_CREAT | O_WRONLY, 0644);
		close(2);
		dup(fd);
	}
	/*> �����̷����� ����� ���*/
	if (useOutFile == 1)
	{
		/*stdout�� �ݰ� dup���� ������ ����*/
		int fd = open(outFilePath, O_CREAT | O_WRONLY, 0644);
		close(1);
		dup(fd);
	}
	/*< �����̷����� ����� ���*/
	if (useInFile == 1)
	{
		/*stdin�� �ݰ� dup���� ������ ����*/
		int fd = open(inFilePath, O_RDONLY);
		close(0);
		dup(fd);
	}

	/*path ����*/
	strcpy(path, PREFIX);
	strcat(path, "/");
	strcat(path, arg[0]);

	/*exec ȣ��*/
	execv(path, arg);

	/*exec ���� �� command ��� �� 2�� ����*/
	write(STDOUT_FILENO, arg[0], strlen(arg[0]));
	return 2;
}
