#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    // Read the first command line argument as the number of words to print
    int num_words = atoi(argv[1]);

    // Loop through each line of input
    char line[1000];
    while (fgets(line, sizeof(line), stdin)) {
        // Split the line into words
        char *words[1000];
        int num_words_found = 0;
        char *word = strtok(line, " \n");
        while (word && num_words_found < num_words) {
            words[num_words_found++] = word;
            word = strtok(NULL, " \n");
        }
        
        // Print the first num_words words
        for (int i = 0; i < num_words_found; i++) {
            printf("%s ", words[i]);
        }
        printf("\n");
    }
    
    return 0;
}
