#ifdef __linux__
#  include <getopt.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>   /* umask */ 
#include <fcntl.h>
#include <ctype.h>      /* isalnum, isdigit */
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#ifndef __MY_COMMON_CODE__
#define __MY_COMMON_CODE__

/* ###################################################################### */
/*             Raw Token Parsing and Handling Structures                  */
/* ###################################################################### */

#define CHARACTER_QUEUE_BUFFER_SIZE 50

/*Will Hold All Characters readin*/
struct CHARACTER_QUEUE
{
    char buffer[CHARACTER_QUEUE_BUFFER_SIZE];
    int len;
    struct CHARACTER_QUEUE *next;
};

#  define TOKEN_QUEUE_BUFFER_SIZE 20

struct TOKEN_QUEUE
{
    char *token[TOKEN_QUEUE_BUFFER_SIZE];
    int len;
    struct TOKEN_QUEUE *next;
};

struct FILE_STATE
{
  int fd;
  char readChar[512];
  int readin;
  int sc;
  int lineNumber;

  int newLine;
};

typedef struct FILE_STATE FILE_STATE;

/* *INDENT-OFF*
   ############################################################
   #           Library Prototype for Common Code              #
   ############################################################
   *INDENT-ON* */
#ifndef MY_COMMON_CODE_PROTO

int trimSpaceAndZeroFromEnd(char *s, int sz);
int trimSpaceAndZeroFromBeginToEnd(char *s, int sz);


int setCharField(char *fld, int size, char *src);



int enqueueCharacter(struct CHARACTER_QUEUE **head, char inChar);
int dumpCharQueueToStr(char *str, struct CHARACTER_QUEUE *head);
int freeCharacterQueue(struct CHARACTER_QUEUE **head);
int enqueueToken(struct TOKEN_QUEUE **head, char *token);
char *getTokenNumber(struct TOKEN_QUEUE *head, int tokenNumber);
int freeTokenQueue(struct TOKEN_QUEUE **head);
int fillTokensFromFile(FILE_STATE *fs, struct TOKEN_QUEUE **tokenHead,
		       int delimiter, int tokenLimit,
		       int newLineBreaks,int quote);


#endif
#endif

struct NavTokenMap {
  /*The Token*/
  char *token; 
  int tokenSize;
  
  /*The Value*/
  char *value;
  int valueSize;

  struct NavTokenMap *next;
};

typedef struct NavTokenMap NameValue;

char *findMapKeyValue(NameValue *head,const char *key,int ignoreCase,int startsWith,int KeyValue,int *tokenNumber,int stepTokenNumber);

int PullInRequest(int fd, NameValue **head);

/*TODO get rid of this, make it an option or param.*/
static int UpcaseTokens = 0;

int main(int argc, char * argv[])
{

  int fd = -1;
  int numberOfNodes = 0;
  int i;
  char *ptr = NULL;
  int stack = 0;
  NameValue *map = NULL;
  NameValue *tmp = NULL;
  char searchKey[30];
  
  if (argc < 2)
    {
      printf("USAGE: %s filename\n",argv[0]);
      return (-1);
    }
  
  fd = open(argv[1],O_RDONLY);

  if(fd < 0)
    {
      printf("Cannot open file %s for reading\n",argv[1]);
      return(-1);
    }

  snprintf(searchKey,sizeof(searchKey),"key");

  if(argc > 2)
    {
      snprintf(searchKey,sizeof(searchKey),"%s",argv[2]);
    }
  
  numberOfNodes = PullInRequest(fd, &map);

  printf("Tokens found : %d\n",numberOfNodes);
  /*
  for(tmp = map, i = 0; NULL != tmp ; tmp = tmp->next, i++)
  {
      printf("Node %3d ",i);
      printf("%s = %s \n",tmp->token,tmp->valueSize > 0 ? tmp->value : "[No Value]");
  }
  */

  for(ptr = findMapKeyValue(map,(const char *)searchKey,1,1,0,&stack,1);
      NULL != ptr;
      ptr = findMapKeyValue(map,(const char *)searchKey,1,1,0,&stack,1))
    {
      printf("Stack %3d %s %s\n",stack,ptr,findMapKeyValue(map,(const char *)searchKey,1,1,1,&stack,0));
    }
  
  close(fd);
  
  return (0);
}

char *findMapKeyValue(NameValue *head,const char *key,int ignoreCase,int startsWith,int KeyValue,int *tokenNumber,int stepTokenNumber)
{
  int keyLen = -1;
  NameValue *tmp = NULL;
  int goToToken = -1;
  int grabIt = 0;
  
  if(NULL == key)
    {
      return (NULL);
    }

  if(NULL != tokenNumber)
    {
      if(*tokenNumber < 0)
	{
	  *tokenNumber = 0;
	}
      
      goToToken = *tokenNumber;

      if(stepTokenNumber)
	{
	  *tokenNumber = goToToken + 1;
	}
      else if(KeyValue && !stepTokenNumber)
	{
	  /*We are reading using the stack variable we are looking up from*/
	  goToToken--;
	}
    }

  for(tmp = head; NULL != tmp ; tmp = tmp->next)
    {
      if(strlen(key) < 1)
	{
	  grabIt = 1;
	}
      
      if(!grabIt)
	{

	  keyLen = strlen(tmp->token);
	  if(startsWith)
	    {
	      keyLen = strlen(key);
	    }
	  
	  if(ignoreCase)
	    {
	      /*caseless compare*/
	      if(strncasecmp(key,tmp->token,keyLen) == 0)
		{
		  grabIt = 1;
		}
	    }
	  else
	    {
	      /*case compare*/
	      if(strncmp(key,tmp->token,keyLen) == 0)
		{
		  grabIt = 1;
		}		  
	    } 
	}
      
      if (grabIt && (goToToken > 0))
	{
	  goToToken--;
	  grabIt = 0;
	  continue;
	}

      if(grabIt)
	{
	  if(KeyValue)
	    return (tmp->value);
	  else
	    return (tmp->token);
	}
    }

  return (NULL);
}



/*!
 */
int PullInRequest(int fd, NameValue **head)
{

    int num = 0;
    FILE_STATE fs;
    int j;
    struct TOKEN_QUEUE *mytokens = NULL;
    int i,valueLength;
    
    NameValue *map = NULL;
    
    int ret = 0;
    char *ptr = NULL;

    memset((void *)&fs,'\0',sizeof(fs));

    fs.fd = fd;
    
    for(ret = 0; ret > -1; 
	ret = fillTokensFromFile(&fs, 
				 &mytokens, 
				 -1, 
				 0, 
				 1,
				 -1))
      {
	
	if(ret < 1)
	  {
	    continue;
	  }
	/*	
		printf("Tokens : %2d ",ret);
	*/
	for(j = 0 ; j < ret ; j++)
	  {

	    valueLength = strlen(getTokenNumber(mytokens, j));
	    ptr = getTokenNumber(mytokens, j);
	    if((NULL == ptr) || (valueLength) < 1)
	      {
		/*
		  printf("[NULL]");
		*/
	      }
	    else
	      {
		int a = valueLength;
		int b = 0;
		/*
		  printf("[%s]",ptr);
		*/
		a = 0;
		b = 0;
		
		num++;
		for(i = 0 ; i < valueLength ; i++)
		  {
		    if('=' == *(ptr + i))
		      {
			/*Absolute Values*/
			a = i;
			/*b unset if no = sign*/
			b = valueLength - i - 1;
			break;
		      }
		  }
		/*values without tokens do not count*/
		if(a > 0)
		  {

		    int aplusone = a + 1;
		    int bplusone = b + 1;
		    
		    NameValue *tmp = NULL;
		    
		    tmp = (NameValue *)malloc(sizeof(NameValue));

		    tmp->tokenSize = a;
		    
		    tmp->token = (char *)malloc(sizeof(char) * aplusone);
		    memset(tmp->token,'\0',aplusone);
		    
		    tmp->valueSize = b;
		    
		    tmp->value = (char *)malloc(sizeof(char) * bplusone);
		    memset(tmp->value,'\0',bplusone);

		    snprintf(tmp->token,aplusone,"%*.*s",tmp->tokenSize,tmp->tokenSize,ptr);		    
		    if(tmp->valueSize > 0)
		      {
			snprintf(tmp->value,bplusone,"%*.*s",tmp->valueSize,tmp->valueSize,(ptr + i + 1));		    
		      }

		    /*add to head, if head exists, add to tail*/

		    tmp->next = NULL;
		    /*
		    printf("%s = %s \n",tmp->token,tmp->valueSize > 0 ? tmp->value : "[No Value]");
		    */
		    if (NULL == *head)
		      {
			*head = tmp;
		      }
		    else
		      {
			NameValue *hold = NULL;
			hold = *head;
			while ( NULL != hold->next )
			  {
			    hold = hold->next;
			  }
			hold->next = tmp;
		      }
		  }
	      }
	  }
	/*
	  printf("\n");
	*/
	freeTokenQueue(&mytokens);
      }
    /* 
       close(fs.fd);
    */
    return num;
}

/* *INDENT-OFF*
   ############################################################
   #              Input Options and Validation                #
   ############################################################
   *INDENT-ON* */
/*!
  \brief trim the spaces and null characters from a byte
  sequence replacing them with just spaces.
  
  \param[in] s the character buffer
  \param[in] z the size of the buffer
  \return 0 if ok, -1 if s is NULL or sz < 1
 */
int trimSpaceAndZeroFromEnd(char *s, int sz)
{
    int i;

    if (s == NULL || sz < 1)
    {
        return -1;
    }

    for (i = (sz - 1); i > -1; i--)
    {
        if (s[i] == '\0' || s[i] == ' ' || s[i] == '\n' || s[i] == '\r')
        {
            s[i] = '\0';
            continue;
        }
        break;
    }

    return 0;
}


/*!
  \brief clean out the white space from the front and back
  \param[in] s the string to clean
  \param[in] sz the size of the buffer
  \retval 0 success
  \retval -1 failure
 */
int trimSpaceAndZeroFromBeginToEnd(char *s, int sz)
{
    int i, j;

    if (NULL == s || sz < 1)
    {
        return (-1);
    }

    /*cut off white space on front */
    for (i = 0; i < (sz - 1) &&
         ('\0' == s[0] ||
          ' ' == s[0] || '\n' == s[0] || '\r' == s[0] || '\t' == s[0]); i++)
    {
        for (j = 1; j < sz; j++)
        {
            s[(j - 1)] = s[j];
            s[j] = ' ';
        }
    }
    return (trimSpaceAndZeroFromEnd(s, sz));
}

/*!
  \brief copy a CSV token to a fixed size buffer
  \param[out] fld the target character field
  \param[in] size the size of fld
  \param[in] src a null terminated string
  \return the size of the string
  \retval -1 some issue with fld or size of fld
 */
int setCharField(char *fld, int size, char *src)
{
    if (NULL == fld || size < 1)
    {
        return (-1);
    }

    memset(fld, '\0', size);

    if ((NULL == src) || (strlen(src) < 1))
    {
        return (1);
    }

    snprintf(fld, size, "%s", src);

    return (strlen(fld));
}




/* *INDENT-OFF*
   ############################################################
   #                Parsing and Data Handling                 #
   ############################################################
   *INDENT-ON* */

/*!
  \brief shoves a character onto an expanding array
  \param[out] head pointer to a pointer
  \param[in] inChar the character we wish to save
  \retval the length of queue
*/
int enqueueCharacter(struct CHARACTER_QUEUE **head, char inChar)
{
    struct CHARACTER_QUEUE *tmp = NULL;
    int block = 0;

    if (NULL == *head)
    {
        tmp = (struct CHARACTER_QUEUE *)malloc(sizeof(struct CHARACTER_QUEUE));
        memset((char *)tmp, '\0', sizeof(struct CHARACTER_QUEUE));
        tmp->len = 0;
        tmp->next = NULL;
        *head = tmp;
    }

    tmp = *head;

    while (tmp->next != NULL)
    {
        tmp = tmp->next;
        block++;
    }

    if (tmp->len > (CHARACTER_QUEUE_BUFFER_SIZE - 1))
    {
        /*we need a new block */
        tmp->next = (struct CHARACTER_QUEUE *)
                            malloc(sizeof(struct CHARACTER_QUEUE));
        memset((char *)tmp->next, '\0', sizeof(struct CHARACTER_QUEUE));
        tmp->next->len = 0;
        tmp->next->next = NULL;
        tmp = tmp->next;
        block++;
    }

    tmp->buffer[tmp->len++] = inChar;

    return (block * CHARACTER_QUEUE_BUFFER_SIZE + tmp->len);
}

/*!
  \brief dump the queue to a string
  \param[in] str is a string that is at least size + 1 of the characters
  in the character queue, it is up to the caller to make sure this is the
  case
  \param[in] head a point to a character queue
  \retval the total number of characters copied
 */
int dumpCharQueueToStr(char *str, struct CHARACTER_QUEUE *head)
{
    char *ptr = NULL;
    struct CHARACTER_QUEUE *tmp = NULL;
    int total = 0;

    tmp = head;
    ptr = str;

    while (tmp != NULL)
    {
        /*Take it on faith that the len is 1.. CHARACTER_QUEUE_BUFFER_SIZE */
        memcpy(ptr, tmp->buffer, tmp->len);
        total += tmp->len;
        /*Advance Pointer */
        ptr += tmp->len;
        tmp = tmp->next;
    }

    return (total);
}

/*!
  \brief frees the character queue
  \param[in] head pointer to a pointer
  \return the number of nodes freed
 */
int freeCharacterQueue(struct CHARACTER_QUEUE **head)
{
    struct CHARACTER_QUEUE *tmp = NULL;
    int ret = 0;

    if (NULL == *head)
    {
        return (0);
    }

    tmp = *head;
    ret = 1 + freeCharacterQueue(&tmp->next);

    free(*head);

    *head = NULL;
    return (ret);
}

/*!
  \brief put a parsed token into the token queue
  \param[out] head pointer to a pointer this is the queue
  that is modified when tokens are pushed to it
  \param[in] token a charactwer string to save t othe Queue
  \return the number of token in the queue
 */
int enqueueToken(struct TOKEN_QUEUE **head, char *token)
{
    struct TOKEN_QUEUE *tmp = NULL;
    int block = 0;

    if (NULL == *head)
    {
        tmp = (struct TOKEN_QUEUE *)malloc(sizeof(struct TOKEN_QUEUE));
        memset((char *)tmp, '\0', sizeof(struct TOKEN_QUEUE));
        tmp->len = 0;
        tmp->next = NULL;
        *head = tmp;
    }

    tmp = *head;

    while (tmp->next != NULL)
    {
        tmp = tmp->next;
        block++;
    }

    if (tmp->len > (TOKEN_QUEUE_BUFFER_SIZE - 1))
    {
        /*we need a new block */
        tmp->next = (struct TOKEN_QUEUE *)malloc(sizeof(struct TOKEN_QUEUE));
        memset((char *)tmp->next, '\0', sizeof(struct TOKEN_QUEUE));
        tmp->next->len = 0;
        tmp->next->next = NULL;
        tmp = tmp->next;
        block++;
    }

    if (NULL != token && strlen(token) > 0)
    {
        tmp->token[tmp->len] =
                            (char *)malloc(sizeof(char) * (strlen(token) + 1));
        memset(tmp->token[tmp->len], '\0', strlen(token) + 1);
        memcpy(tmp->token[tmp->len], token, strlen(token));
    }
    else
    {
        /*0 Length String */
        tmp->token[tmp->len] = (char *)malloc(sizeof(char) * 1);
        tmp->token[tmp->len][0] = '\0';
    }

    /*increase Number of Tokens */
    tmp->len++;

    return (block * TOKEN_QUEUE_BUFFER_SIZE + tmp->len);
}

/*!
  \brief Get the Token at position i starting from 0
  
  \param[in] head pointer to the token Queue
  \param[in] tokenNumber starting from position 0 get this given 
  token number
  \retval NULL tokenNumber Less than 0, head is NULL, or tokenNumber 
  out of range
  \return a valid character pointer to a NULL Terminated String
*/
char *getTokenNumber(struct TOKEN_QUEUE *head, int tokenNumber)
{
    struct TOKEN_QUEUE *tmp = NULL;
    int counter = 0;

    if (tokenNumber < 0)
    {
        return (NULL);
    }

    if (NULL == head)
    {
        return (NULL);
    }

    tmp = head;

    while (NULL != tmp)
    {
        if (counter >= tmp->len)
        {
            counter = 0;
            tmp = tmp->next;
            continue;
        }

        if (tokenNumber < 1)
        {
	    return ((char *)tmp->token[counter]);
        }

        counter++;
        tokenNumber--;
    }

    return ((char *)NULL);
}

/*!
  \brief this routine will free a token queue
  \param[in] head pointer to a pointer of token queue, each
  time will be freed and set to NULL or Zero
  \return the number of tokens that were on the Queue
 */
int freeTokenQueue(struct TOKEN_QUEUE **head)
{
    struct TOKEN_QUEUE *tmp = NULL;
    int i;
    int ret = 0;

    if (NULL == *head)
    {
        return (0);
    }

    tmp = *head;
    ret = freeTokenQueue(&tmp->next);

    for (i = 0; i < tmp->len; i++)
    {
        if (NULL != tmp->token[i])
        {
            free(tmp->token[i]);
            tmp->token[i] = NULL;
            ret++;
        }
    }

    tmp->len = 0;
    tmp->next = NULL;

    free(*head);

    *head = NULL;
    return (ret);
}

/*!
  \brief This routine reads tokens from a file.
  Will parse tokens out of the file, Blank lines are ignored. If
  \param[in] fd an int file descriptor pointing to a file that we move 
  through and parse the tokens from 
  \param[out] tokenHead a pointer to a pointer that the tokens that are 
  parsed out are stored to. 
  \param[in] delimiter a integer that says what the delimiter of the
  tokens are, if set to less than 0 the delimiter is ignored.
  \param[in] tokenLimit an int that says only take up to this many
  tokens from the file, if set to less than 1 then all tokens will
  be taken in.
  \param[out] lineNumber pointer to an integer, will be increased whenever
  a new line character is encountered.
  \param[in] newLineBreaks, if set to 1 we will return  from the routine
  when we encounter a new line character
  \param[in] quote, integer if greater than -1 then they char will be used
  as a quote character.
  \retval -1 file not open
  \retval -99 EOF
  \retval >= 0 number of CSV tokens on line 
 */
int fillTokensFromFile(FILE_STATE *fs, struct TOKEN_QUEUE **tokenHead,
		       int delimiter, int tokenLimit,
		       int newLineBreaks,int quote)
{

    char *tokenBuff = NULL;
    struct CHARACTER_QUEUE *head = NULL;

    int numTokens = 0;

    int size = 0;
    int ret = 0;

    int inQuote = 0;
    int quoteDepth = 0;
    int numQuotes = 0;

    if (fs->fd < 1)
    {
        return (-1);
    }

    /*try to read */
    if (fs->readin < 1 || fs->sc >= fs->readin)
    {
        fs->readin = read(fs->fd, fs->readChar, sizeof(fs->readChar));
        fs->sc = 0;
    }

    /*EOF Test */
    if (fs->readin < 1)
    {
         /*EOF*/ return (-99);
    }

    for (;;)
    {

        if (fs->sc >= fs->readin)
        {
            fs->readin = read(fs->fd, fs->readChar, sizeof(fs->readChar));
            if (fs->readin < 1)
            {
                break;
            }
            fs->sc = 0;
        }

	if((quote > -1) && ((char)quote == fs->readChar[fs->sc]))
	  {
	    
	    inQuote = (inQuote + 1) % 2;
	      
	    numQuotes++;
	    if(2 == numQuotes)
	      {
		/*Enqueue Quoted Quote*/
		if(quoteDepth > 0)
		  {
		    /*We Do not upcase quote chars normally*/
		    if(2 == UpcaseTokens)
		    {
			size = enqueueCharacter(&head, toupper(quote));
		    }
		    else
		    {
			/*Do Not Uppercase*/
			size = enqueueCharacter(&head, quote);
		    }
		    numQuotes = 0;
		  }
		else
		  {
		    /*Just in case a quote is next*/
		    numQuotes = 1;
		  }
	      }  

	    if(!inQuote)
	      {
		quoteDepth++;
	      }

	    fs->sc++;
	    continue;
	  }

	numQuotes = 0;

        if ('\n' == fs->readChar[fs->sc])
        {
            /*increase the line count */
            fs->lineNumber = (fs->lineNumber) + 1;
        }

        if (((!inQuote) && ( (delimiter > -1) && ((char)delimiter == fs->readChar[fs->sc]) ) ) ||
            '\r' == fs->readChar[fs->sc] || '\n' == fs->readChar[fs->sc])
        {

	    /*Reset Quote Vars*/
	    inQuote = 0;
	    quoteDepth = 0;
	    numQuotes = 0;

            if (fs->newLine)
            {
                /*multiple new line characters */
                fs->sc++;
                continue;
            }

            if (size > 0)
            {
                /*Malloc tokenBuff +1 Copy Character Queue to Buffer */
                tokenBuff = (char *)malloc(sizeof(char) * (size + 1));
                memset(tokenBuff, '\0', size + 1);
                dumpCharQueueToStr(tokenBuff, head);
                enqueueToken(tokenHead, tokenBuff);
                /*Clear Character Queue */
                ret = freeCharacterQueue(&head);
                head = NULL;
                /*printf("TOKEN : %s\n", tokenBuff); */
            }
            else if ((delimiter > -1) && (delimiter == fs->readChar[fs->sc]) )
            {
                enqueueToken(tokenHead, "\0");
            }

            if ('\r' == fs->readChar[fs->sc] || '\n' == fs->readChar[fs->sc])
            {
                /*if the token breaker was a new line character 
                   then we mark it so in case multiple new line characters */
                fs->newLine = 1;
            }

            /*Assign tokenBuff pointer to Node (enqueue) */

            size = 0;
            free(tokenBuff);
            tokenBuff = NULL;

            fs->sc++;
            size = 0;
            /*Up the Number of Tokens */
            numTokens++;
            if (tokenLimit > 0)
            {
                if (numTokens >= tokenLimit)
                {
                    return (numTokens);
                }
            }

	    if(newLineBreaks && fs->newLine)
	      {
		return (numTokens);
	      }

            continue;
        }

        /*Enqueue readChar[sc] */
	if(0 != UpcaseTokens)
	{
	    size = enqueueCharacter(&head, toupper(fs->readChar[fs->sc]));
	}
	else
	{
	    size = enqueueCharacter(&head, fs->readChar[fs->sc]);
	}
        fs->newLine = 0;
        fs->sc++;
    }

    if (size > 0)
    {

        /*Malloc tokenBuff +1 Copy Character Queue to Buffer */
        tokenBuff = (char *)malloc(sizeof(char) * (size + 1));
        memset(tokenBuff, '\0', size + 1);
        dumpCharQueueToStr(tokenBuff, head);
        enqueueToken(tokenHead, tokenBuff);
        /*Clear Character Queue */
        ret = freeCharacterQueue(&head);
        head = NULL;
        /*printf("(last) TOKEN : %s\n", tokenBuff); */

        /*printf("[%s]\n",lbuff); */
        free(tokenBuff);
        tokenBuff = NULL;
        numTokens++;
    }

    return (numTokens);
}
