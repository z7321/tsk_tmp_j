#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cjson/cJSON.h>

#define true 1
#define false 0

/**
 * retrieves user supplied destinations for file input and output
 * 
 * @param argc number of arguments given by the user
 * @param argv array of argument given by the user
 * @param iDest pointer to store input file destination
 * @param oDest pointer to store output file destination
 * @return int 1 if true otherwise 0
 */
int argumentsToVariables(int argc, char **argv, char *iDest, char *oDest)
{
    int strLen;
    int strCopied = 0;
    // checking every given argument for -j or -o, getting the size then copying,
    // increasing size by 1 for null terminator. Afterwards linking to a pointer
    // and increasing the counter of string copied.
    for(int i = 0; i<argc; i++)
    {
         if (strcmp(argv[i], "-j") == 0)
         {
             strLen = strlen(argv[i+1]);
             if(strLen<255)
             {
                 strncpy(iDest, argv[i+1], strLen+1);
                 strCopied++;
             }
         }
         else if (strcmp(argv[i], "-o") == 0)
         {
             strLen = strlen(argv[i+1]);
             if(strLen<255)
             {
                 strncpy(oDest, argv[i+1], strLen+1);
                 strCopied++;
             }
         }
    }

    // checks if user used all the necessary flags
    if(strCopied==2)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * retrieves buffer with size from user specified file
 * 
 * @param fileName specified by the user
 * @param bufferSize pointer to store buffer size
 * @return char* buffer with the data gotten from file
 */
char *inputFileToBuffer(char *fileName, long *bufferSize)
{
    FILE *file;
    unsigned long int fileSize;
    char *buffer;

    file = fopen(fileName, "rb");
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    buffer = malloc(fileSize + 1);
    fseek(file, 0, SEEK_SET);

    fread(buffer, fileSize, 1, file);
    fclose(file);
    buffer[fileSize] = '\0';

    *bufferSize = fileSize;
    return buffer;
}

/**
 * creates a json template which is then populated by the buffer data.
 * Coverted into a new buffer with an updated size pointer for the new data.
 * 
 * @param buffer retrieved from user specified file
 * @param bufferSize pointer linked to a new return buffer size
 * @return char* json buffer with trimmed tabs
 */
char *formatBufferToJSON(char *buffer, long *bufferSize)
{
    // creating main and secondary JSON objects and attaching them to the main one.
    // Declaring and initializing JSON string.
    cJSON *data = cJSON_CreateObject();
    cJSON *board = cJSON_AddObjectToObject(data, "board");
    cJSON *pcb = cJSON_AddObjectToObject(board, "pcb");
    cJSON *name = NULL; 
    cJSON *revision = NULL; 
    cJSON *productionDate = NULL;
    cJSON *productionLocation = NULL; 
    cJSON *product = NULL; 
    cJSON *serialNo = NULL;

    // declaring temporary variables for comparision and value assignment
    char tempObjectName[254];
    char tempObjectProperty[254];
    int constructingObjectName = true;

    // looping till null terminator is reached, constructing null terminated strings
    // from buffer using '=' or '\n' as a stop point.
    for(int i = 0, c = 0; buffer[i]!='\0'; i++)
    {
        if(constructingObjectName)
        {
            tempObjectName[c] = buffer[i];
            c++;
        }
        else
        {
            tempObjectProperty[c] = buffer[i];
            c++;
        }

        if(buffer[i+1]=='=')
        {
            tempObjectName[c+1] = '\0';
            constructingObjectName = false;
            c = 0;
            i++;
        }
        else if(buffer[i+1]=='\n'||buffer[i+1]=='\0')
        {
            tempObjectProperty[c+1] = '\0';
            constructingObjectName = true;
            c = 0;
            i++;

            // using the temporary values in variables to check if the
            // value in user file match the criteria. If it does assign
            // the second associated variable value to specific json variable
            if (strcmp(tempObjectName, "PRODUCT_ID") == 0)
            {
                product = cJSON_CreateString(tempObjectProperty);
            }
            else if (strcmp(tempObjectName, "SERIAL_NO") == 0)
            {
                serialNo = cJSON_CreateString(tempObjectProperty);
            }
            else if (strcmp(tempObjectName, "PCB_REVISION") == 0)
            {
                revision = cJSON_CreateString(tempObjectProperty);
            }
            else if (strcmp(tempObjectName, "PCB_NAME") == 0)
            {
                name = cJSON_CreateString(tempObjectProperty);
            }
            else if (strcmp(tempObjectName, "PCB_PROD_DATE") == 0)
            {
                productionDate = cJSON_CreateString(tempObjectProperty);
            }
            else if (strcmp(tempObjectName, "PCB_PROD_LOCATION") == 0)
            {
                productionLocation = cJSON_CreateString(tempObjectProperty);
            }

            // reset the char data in the arrays before looping again
            memset(tempObjectName, 0, sizeof(tempObjectName));
            memset(tempObjectProperty, 0, sizeof(tempObjectProperty));
        }
    }

    // populating json objects with string in a certain order to match a template
    cJSON_AddItemToObject(pcb, "name", name);
    cJSON_AddItemToObject(pcb, "revision", revision);
    cJSON_AddItemToObject(pcb, "production_date", productionDate);
    cJSON_AddItemToObject(pcb, "production_location", productionLocation);
    cJSON_AddItemToObject(board, "product", product);
    cJSON_AddItemToObject(board, "serial_no", serialNo);

    // generating a new buffer that is in json format
    char *jsonBuffer = cJSON_Print(data);

    // freeing variables that won't be used anymore
    cJSON_Delete(data);

    // calculating size using a loop with null terminate as a stop point.
    // Also converting new buffer tabs to simple spaces
    *bufferSize = 0;
    for(int i = 0; jsonBuffer[i]!='\0'; i++)
    {
        *bufferSize += 1;
        if(jsonBuffer[i]=='\t')
        {
            jsonBuffer[i] = ' ';
        }
    }

    return jsonBuffer;
}

/**
 * using the finished buffer to create and populate a json file
 * 
 * @param fileName specified by the user to create and populate
 * @param buffer finished to be used to write a file
 * @param bufferSize to write the correct amount of bytes in the file
 */
void outputBufferToFile(char *fileName, char *buffer, long bufferSize)
{
    FILE *file;
    file = fopen(fileName, "wb");
    fwrite(buffer, sizeof(char), bufferSize, file);
    fclose(file);
}

int main(int argc, char *argv[])
{

     // if argument specified amount is bigger then 4, we run our program otherwise return 1 as
     // failure, the application's name counts as a argument 
    if(argc>4)
    {
        int variablesNotEmpty;
        char inputFileDestination[255], outputFileDestination[255];
        long bufferSize;

        // retrieve user submitted input and output destinations, if they are not empty returns 1
        variablesNotEmpty = argumentsToVariables(argc, argv, inputFileDestination, outputFileDestination);
        if(variablesNotEmpty)
        {
            // returns buffer contents and size from user specified file 
            char *buffer = inputFileToBuffer(inputFileDestination, &bufferSize);

             // buffer data used to create a new json format buffer and update size to be
             // used by the new buffer
            char *outBuffer = formatBufferToJSON(buffer, &bufferSize);

             // new buffer and size used to create a populated json file, to a user
             // specified file name
            outputBufferToFile(outputFileDestination, outBuffer, bufferSize);

            // freeing buffers that won't be used
            free(buffer);
            free(outBuffer);

            return 0;
        }
    }

    return 1;
}