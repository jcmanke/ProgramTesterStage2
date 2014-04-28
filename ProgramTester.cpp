/******************************************************************************
 *
 *  Simple Program Tester
 *
 *  Authors: Colter Assman, Samuel Carroll, Shaun Gruenig
 *  Adventure Line Authors: Erik Hattervig, Andrew Koc, Jonathan Tomes
 *  Lounge Against the Machine Authors: Joe Manke, Adam Meaney
 * CS470 - Software Engineering
 * Dr. Logar
 * March 24, 2014
 *
 * Program Description:
 *     This program is to be placed into a directory containing
 *     subdirectories with .cpp files, associated .tst files which
 *     contain test input for the program contained in the cpp file,
 *     and .ans files that contain the expected output associated with
 *     said input. This program will then find the specified cpp file,
 *     compile and run the program against all given test cases, and
 *     output a log file in the same directory as the .cpp file.
 *
 *     A more in-depth description of this program can be found in
 *     the accompanying documentation.
 *
 *
******************************************************************************/

int MAX_TIMEOUT = 60;

/////////includes//////////////////////////////////////////////////////////////
#include <iostream>
#include <stdlib.h>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <fstream>
#include <ctime>
#include <sys/wait.h>
#include <sys/types.h>
#include <algorithm>
#include <sstream>

using namespace std;


//////////structs//////////////////////////////////////////////////////////////
struct data_struct
{
    int passed;
    int failed;
    int total;
    bool crit_failed;
};


///////////function prototypes/////////////////////////////////////////////////
bool compile( string progName );
void FinalLogWrite( std::ofstream &fout, string name, int numPassed, 
    int numTotal);
void generateTestCases( string rootDir );
void generateInts(int numberOfTests, int numberOfArgs);
void generateFloats(int numberOfTests, int numberOfArgs);
void generateStrings(int numberOfTests, int numberOfArgs);
void generateMenu(int numberOfTests, string rootDir, string testDir);
bool stringEndsWith(string fullString, string ending);
bool isAllDigits(string toCheck);
string get_time ();
void menuLoop( string rootDir );
int run_diff ( string file1, string file2 );
bool RunTestCase(string exec, string test_case, string curr_dir, 
	string student_dir, data_struct *rec, ofstream &log);
void StudentLogWrite( std::ofstream &fout, string testName, int passedStatus );
void run_gprof(string progName, ofstream &log);
void studentDirCrawl( string rootDir );
void testCrawl( string testPath, string exePath, ofstream &studentLog, 
  data_struct *rec, string studentPath );
void askForTimeout();
bool manualDiff(string file1, string file2);
bool testResults(string ansWord, string outWord);
void run_gcov(string progName, ofstream &log);

/******************************************************************************
 *
 *  main function
 *
 *  @Description:
 *  this function is the main entryway into the program. It initializes
 *  all necessary variables and calls all necessary functions to accomplish
 *  the tasks of the program.  It calls the parse_directory function to
 *  find the target cpp file, which in turn calls the find_tst function
 *  to find all .tst files associated with that target and run the test
 *  inputs against the target program. Main() then compiles all the
 *  statistics recorded during the tests and outputs the stats to a log
 *  file.
 *
 *  Parameters:
 *      int argc - number of arguments passed in
 *      char* argv[] - array of c strings containing CL arguments
 *
 *  @Author: Shaun Gruenig
 *  @Author: Erik Hattervig
 *
******************************************************************************/
int main( int argc, char* argv[] )
{
    char cCurrentPath[FILENAME_MAX];
    string rootDir;


    if ( argc < 2 )
    {
        getcwd( cCurrentPath, sizeof(cCurrentPath) );
        rootDir = cCurrentPath;
    }
    else
    {
        rootDir = argv[1];
    }
    menuLoop(rootDir);
}

/*******************************************************************************
 * @Function compile
 * @Author: Jonathan Tomes
 * 
 * @Description:
 * This function will compile the first cpp file it finds in a directory.
 * It will compile to either the given program name (progName) or to the default
 * a.out that g++ uses. It assumes that it is in the directory already and that
 * there is only one cpp file to compile.
 * 
 * @Param[in] progName - string for the program name to compile to
 *              if empty it will assume to use default g++ program name.
 * @return bool true - compile successful
 *              false - compile failed.
*******************************************************************************/
bool compile( string progName )
{
    char cCurrentPath[FILENAME_MAX];
    DIR * dir;
    struct dirent* file;        //file entity struct from dirent.h
    string filename;
    bool foundFlag = false;
    string cppFile;
    string command;
    
    getcwd( cCurrentPath, sizeof(cCurrentPath) );
    //start looking for the cpp 
    dir = opendir( cCurrentPath );
    
    while( ( file = readdir(dir) ) != NULL && !foundFlag )
    {
        //get file name
        filename = file -> d_name;
        //skip over "." and ".."
        if( filename != "." && filename != ".." )
        {
            //check if the file is a cpp file.
            if( stringEndsWith(filename, ".cpp") == true )
            {
                cppFile = filename;
                foundFlag = true;
            }
        }
    }
    
    //check to see if a cpp file was found at all.
    if(!foundFlag)
    {
        cout << "Could not find a cpp file in: " << cCurrentPath << endl;
        return false;
    }
    
    //build compile command
    //coverage flags first
    command = "g++ -fprofile-arcs -ftest-coverage ";
    
    //check to see if a program name was specified.
    if( !progName.empty() )
    {
        command += "-o " + progName + " ";
    }
    
    command += cppFile;
    
    //profiling flag
    command += " -pg";
    
    //execute the command.
    system( command.c_str() );
    
    return true;
}

/******************************************************************************
 *
 * FinalLogWrite
 *
 * @Author: Jonathan Tomes
 *
 * @Description:
 *  This function will write the final log entry to a student file. or to the
 *  class log file. It expects the stream to write to, the students name
 *  and the number of tests their program passed, and the number o tests
 *  actually used to test their code. If the number of tests passed is zero
 *  it assumes a critical test was failed so instead of printing the percent
 *  of passed tests it just prints "FAILED" next to their name.
 *
 *  @param[in] fout - the stream to write to.
 *  @param[in] name - the students name.
 *  @param[in] numPassed - the number of tests passed, if zero assumes failed
 *      critical test.
 *  @Param[in] numTotal - the total number of tests.
******************************************************************************/

void FinalLogWrite( std::ofstream &fout, string name, data_struct *rec )
{
    //calculate the percentage passed.
    float perPassed;
    perPassed = 100 * rec->passed/rec->total;
    
    //check to see if they passed crit Tests
    if(rec->crit_failed)
    {
            //They failed!
            fout << name << ": "<< "FAILED" << std::endl;
    }
    else
    {
        //Prints student name and percentage passed.
        fout << name << ": " << perPassed << "% passed" << std::endl;
    }

    return;
}

/******************************************************************************
 * @Function generateTestCases
 * @author Erik Hattervig
 * @author Joe Manke
 * 
 * @Description:
 * Asks the user for the type of test data, the number of test cases to
 * generate, and the number of arguments in each test case, it then generates
 * the test cases in a GeneratedTests folder and then call for the test cases
 * for run through the golden program.
 *
 * @param[in] rootDir - the path to the root directory
 *
 *****************************************************************************/
void generateTestCases( string rootDir )
{
    string inputType;       // used to store the type of input
    string inputNumber;     // used to store the number of cases to generate
    string inputArgs;       // used to store the number of args in a test case
    string testDir;         // the directory the test cases are in
    string filename;        // used to store filenames when creating the files
    string command;         // used to create strings for commands
    int numberOfTests;      // integer value of number of test cases
    int numberOfArgs;       // integer value of number of args per test case
    int i,j;
    ofstream fout;          // out file stream when populating test cases
    char buffer[10];        // used in int to string conversion
    
    do
    {
        // Print menu for generating test cases too allow for
        // the options of integers or floats
        cout << "\nType of test data?\n  1: Integer\n  2: Float\n  3: String\n  4: Menu\n";
        cout << "Selection: ";
        
        cin >> inputType;
        
		if( inputType != "1" && inputType != "2" && inputType != "3" && inputType != "4" )
        {
            cout << "Please enter a valid option." << endl;
        }
    
    }while( inputType != "1" && inputType != "2" && inputType != "3" && inputType != "4" );
    
    
    do
    {
        // Print menu for number of test cases created
        cout << "\nWhat number of test cases would you like to generate? ";
        
        cin >> inputNumber;
        // Make sure the input is a number
        if ( !isAllDigits(inputNumber) )
        {
            cout << "Please enter a valid number." << endl;
        }
    }while( !isAllDigits(inputNumber) );
     
    numberOfTests = atoi( inputNumber.c_str() ); 
     
    if(inputType != "4")
    {    
        do
        {
            // Print Menu for number of arguments in each test case
            cout << "\nWhat number of arguments would you like in each test case? ";
            cin >> inputArgs;
            
            if ( !isAllDigits(inputArgs) )
            {
                cout << "Please enter a valid number:\n";
            }
            
        }while( !isAllDigits(inputArgs) );
        
        numberOfArgs = atoi( inputArgs.c_str() );
    }
    
    // change into the test folder
    testDir = rootDir + "/test";
    chdir( testDir.c_str() );
    
    // delete the generated test in the test folder
    system( "rm -rf GeneratedTests/" );
    // recreate generated tests folder and change into it.
    system( "mkdir GeneratedTests" );
    testDir = testDir + "/GeneratedTests";
    chdir( testDir.c_str() );
    
    if( inputType == "1" )
    {
		generateInts(numberOfTests, numberOfArgs);
    }
    else if ( inputType == "2" )
    {
		generateFloats(numberOfTests, numberOfArgs);
    }
    else if ( inputType == "3" )
    {
        generateStrings(numberOfTests, numberOfArgs);
    }
    else if ( inputType == "4" )
    {
        generateMenu(numberOfTests, rootDir, testDir);
    }
    
    // run tests through golden cpp
    // compile golden cpp
    chdir( rootDir.c_str() );
    compile( "" );
    
    // change back to the test directory
    chdir( testDir.c_str() );
    
    // run cases though a.out
    for( i = 0 ; i < numberOfTests ; i++ )
    {
        chdir( testDir.c_str() );
        // create the .ans file
        sprintf( buffer, "%d", i);
        filename = "Test_" + (string)buffer + ".ans";
        command = "touch " + filename;
        system( command.c_str() );
        
        // run through the executable
        chdir( rootDir.c_str() );
        command = "./a.out < test/GeneratedTests/Test_" + (string)buffer;
        command += ".tst > test/GeneratedTests/Test_" + (string)buffer;
        command += ".ans";
        system( command.c_str() );
    }

    return;
}

/******************************************************************************
 * @Function generateInts
 * @author Erik Hattervig
 * @author Joe Manke
 * 
 * @Description:
 * Generates files filled with random integers with values from 0 to 1000.
 *
 * @param[in] numberOfTests - the number of test files to generate
 * @param[in] numberOfArgs - the number of values per file
 *
 *****************************************************************************/
void generateInts(int numberOfTests, int numberOfArgs)
{
	ofstream fout;
	string fileName;
	char * testNum;
	int i, j;

	for( i = 0 ; i < numberOfTests ; i++ )
    {
        sprintf( testNum, "%d", i);
		fileName = "Test_" + (string)testNum + ".tst";
        fout.open( fileName.c_str() );
            
        for( j = 0 ; j < numberOfArgs ; j++ )
        {
            // generate ints, maxed at 1000
            fout << rand() % 1000;
            fout << endl;
        }
        fout.close();
    }

}

/******************************************************************************
 * @Function generateFloats
 * @author Erik Hattervig
 * @author Joe Manke
 * 
 * @Description:
 * Generates files filled with random floating point numbers with values from 
 * 0.0 to 1000.0.
 *
 * @param[in] numberOfTests - the number of test files to generate
 * @param[in] numberOfArgs - the number of values per file
 *
 *****************************************************************************/
void generateFloats(int numberOfTests, int numberOfArgs)
{
	ofstream fout;
	string fileName;
	char * testNum;
	int i, j;

	for( i = 0 ; i < numberOfTests ; i++ )
    {
		sprintf( testNum, "%d", i);
		fileName = "Test_" + (string)testNum + ".tst";
        fout.open( fileName.c_str() );
            
        for( j = 0 ; j < numberOfArgs ; j++ )
        {
            // generate floats, maxed at 1000
            fout << static_cast <float> (rand()) /
            (static_cast <float> (RAND_MAX/1000));
            fout << endl;
        }
        fout.close();
    }
}

/******************************************************************************
 * @Function generateStrings
 * @author Joe Manke
 * 
 * @Description:
 * Asks the user if they want exact or variable length strings, and the exact/
 * maximum length of the strings. Then generates files filled with random
 * strings of lowercase characters.
 *
 * @param[in] numberOfTests - the number of test files to generate
 * @param[in] numberOfArgs - the number of values per file
 *
 *****************************************************************************/
void generateStrings(int numberOfTests, int numberOfArgs)
{
	ofstream fout;
	string fileName, stringType, stringLength;
	char * testNum;
	char letter;
	int length, maxLength;
	int i, j, k;
	bool notANumber = false;

	// determine exact or variable length
	do
    {
		cout << "Would you like exact length or variable length strings?" << endl;
        cout << "1. exact\n2. variable\n";
        cout << "Selection: ";
        cin >> stringType;

		notANumber = !isAllDigits(stringType);
		if (notANumber)
        {
            cout << "Please enter a valid number:\n";
        }
    }while(notANumber || (stringType != "1" && stringType != "2"));
        
	// determine length
	do
	{
		cout << "How long do you want the strings? (max 80) ";
		cin >> stringLength;

		notANumber = !isAllDigits(stringLength);
		if(notANumber)
        {
            cout << "Please enter a valid number:\n";
        }
	}while(notANumber);

	// convert input to integer
	maxLength = atoi(stringLength.c_str());
	if(maxLength > 80)
	{
		maxLength = 80;
	}

	//generate strings
	for(i = 0; i < numberOfTests; i++)
	{
	    //create file
		sprintf( testNum, "%d", i);
		fileName = "Test_" + (string)testNum + ".tst";
        fout.open( fileName.c_str() );
            
        //create word
        for( j = 0 ; j < numberOfArgs ; j++ )
        {
            //determine length
		    if(stringType == "1") //exact length
		    {
			    length = maxLength;
		    }
		    else //variable length
		    {
			    length = rand() % maxLength + 1;
		    }
            
            //generate letters
            for(k = 0; k < length; k++)
			{
				// all lowercase letters
                // a = 97, z = 122
                letter = (char) (rand() % 26 + 97);
                fout << letter; 
			}

            fout << endl;
        }
        fout.close();
	}
}

/******************************************************************************
 * @Function generateMenu
 * @author Joe Manke
 * 
 * @Description:
 * Finds a .spec file and uses it to generate test cases for menu-driven 
 * programs. Each line of the .spec file should be formatted as follows:
 *  <menu option> {type = "int"|"double"}
 *
 * @param[in] numberOfTests - the number of test files to generate
 * @param[in[ rootDir - the directory being tested, where the .spec file 
 *                      is located
 * @param[in] testDir - where to write the .tst file
 *****************************************************************************/
void generateMenu(int numberOfTests, string rootDir, string testDir)
{
    char cCurrentPath[FILENAME_MAX];
    DIR * dir;
    struct dirent* file;        //file entity struct from dirent.h
    string fileName;
    bool foundFlag = false;
    string specFile, specLine, value;
    ifstream fin;
    ofstream fout;
    char testNum[10];
    int lastIndex, index;
    
    //start looking for the .spec file
    chdir( rootDir.c_str() );
    dir = opendir( rootDir.c_str() );
    
    while( ( file = readdir(dir) ) != NULL && !foundFlag )
    {
        //get file name
        fileName = file -> d_name;
        //skip over "." and ".."
        if( fileName != "." && fileName != ".." )
        {
            //check if the file is a .spec file.
            if( stringEndsWith(fileName, ".spec" ) )
            {
                specFile = rootDir + "/" + fileName;
                foundFlag = true;
            }
        }
    }
    
    //check to see if a .spec file was found at all.
    if(!foundFlag)
    {
        cout << "Could not find a .spec file in: " << cCurrentPath << endl;
        return;
    }

    //generate the tests
    chdir( testDir.c_str() );
    
    for( int i = 0; i < numberOfTests; i++)
    {
        fin.open( specFile.c_str() );
        
		sprintf( testNum, "%d", i);
		fileName = "Test_" + (string)testNum + ".tst";
        fout.open( fileName.c_str() );
        
        while( getline(fin, specLine) )
        {
            lastIndex = 0;
            
            while( lastIndex < specLine.length() && lastIndex != string::npos )
            {
                index = specLine.find(" ", lastIndex);
                
                if(index == string::npos)
                {
                    break;
                }
                
                value = specLine.substr(lastIndex, index - lastIndex);
                
                if( isAllDigits( value ) )
                {
                    fout << value;
                }
                else if ( value == "int" )
                {
                    fout << rand();
                }
                else if ( value == "double" )
                {
                    fout << (double) rand() / RAND_MAX;
                }
                
                fout << " ";
                
                lastIndex = index + 1;
            }
            
            fout << endl;
        }
        
        fin.close();
        fout.close();
    }
}

/******************************************************************************
 * @Function stringEndsWith
 * @author Joe Manke
 * 
 * @Description:
 * Determines if one string ends with another string. To be used for finding
 * .cpp, .C, and .spec files. Based on code found at
 * http://stackoverflow.com/questions/874134/find-if-string-endswith-another-string-in-c
 *
 * @param[in] fullString - the string being checked
 * @param[in] ending - the string to check with
 *
 * @return bool - true if fullString ends with ending, false if not 
 *****************************************************************************/
bool stringEndsWith(string fullString, string ending)
{
    int fullLength = fullString.length();
    int endLength = ending.length();
    
    if(fullLength >= endLength)
    {
        //check the last endLength characters for ending
        return ( fullString.compare( fullLength - endLength, endLength, ending ) == 0);
    }
    
    return false;
}

/******************************************************************************
 * @Function isAllDigits
 * @author Joe Manke
 * 
 * @Description:
 * Determines if every character in a string is a digit.
 *
 * @param[in] toCheck - the string to check the contents of
 *
 * @return bool - true if all letters are digits, false if not
 *****************************************************************************/
bool isAllDigits(string toCheck)
{
    return ( toCheck.find_first_not_of("0123456789") == string::npos );
}

/******************************************************************************
 *
 *  get_time function
 *
 *  @Description:
 *  This function uses the <time.h> library functions to construct
 *  a string which contains the time and date
 *
 *  Parameters: none
 *
 *  @Author: Shaun Gruenig (referred to cplusplus.com for example code)
 *
******************************************************************************/
string get_time ()
{
    char string_version[1000]; //stores date and time
    time_t raw; //store time in time_t format
    struct tm* formatted; //stores time in struct tm format

    time ( &raw ); //gets time in seconds
    formatted = localtime ( &raw ); //formats time as struct

    //formats time into yyyy-mm-dd_hh:mm:dd
    strftime ( string_version, 100, "%F_%X", formatted );
    return ( string ) string_version;
}

/******************************************************************************
 * @Function menuLoop
 * @author Erik Hatterivg
 *
 * @Description:
 * This function prompts the user for a option, its options will include running
 * the program as normally, generating test cases, or exiting.
 *
 * @param[in] rootDir - A string containing a path to the root directory
 *
 *****************************************************************************/
void menuLoop( string rootDir )
{
    string input;
    
    // Loop till we return
    while(true)
    {
        // print out the menu
        cout << "Please select an option:\n  1: Run test cases\n";
        cout << "  2: Generate test cases\n  3: Exit\nSelection: ";
    
        cin >> input;

        if( input == "1" )
        {
            askForTimeout();
            // run the program as normal
            cout << "Running tests, please wait." << endl;
            studentDirCrawl(rootDir);
        }
        else if( input == "2" )
        {
            generateTestCases( rootDir );
        }
        else if( input == "3" )
        {
            return;
        }
        else
        {
            cout << "That is not a valid option. Please enter again." << endl;
        }
    }
}

/******************************************************************************
 *
 *  run_diff function
 *
 *  @Description:
 *  This function will build a diff command for the system function. After that
 *  command is built we will run that command and if there is no difference
 *  we return true, and if there is a difference we return false. This will be
 *  used to determine if the program result passed or failed, and then use the
 *  information to write to the log file.
 * 
 *  Parameters:
 *	string file1 - file to be compared with file2
 *	string file2 - file to be compared with file1
 *
 *  @Author: Samuel Carroll
 *  @Author: Adam Meaney
 *
******************************************************************************/

int run_diff ( string file1, string file2 )
{
    string command; //string that will hold the command
    int result; // result of the diff system function

    command = "diff "; // start by writing the diff name and space
    command += "-w -i "; //options to ignore whitespace differences and case.
    command += file1; // first file we want to check
    command += " "; // space between the files
    command += file2; // second file we want to check
    command += " > /dev/null"; // dump output from screen
    
    // run the command function and save the result
    result = system( command.c_str() );

    if ( result == 0 ) // if we get a zero there is no difference between the files
        return 1; // return true so we know the test passed

    if (manualDiff(file1, file2) == true)
        return 2;
    
    return 0;
} // end of run_diff function

/******************************************************************************
 * @Function: StudentLogWrite
 * @Author: Jonathan Tomes
 * @Author: Adam Meaney
 *
 * @Description:
 *  Prints the status of a test to the file stream. Status should be weather
 * or not the a test passed (true) or failed (false).
 *
 * @param[in] fout - the stream to write to.
 * @param[in] testName - name of the test.
 * @param[in] passedStatus - 2 - test had a presentation error
                            1 - test passed normally.
 *                          0 - test failed.
******************************************************************************/

void StudentLogWrite( std::ofstream &fout, string testName, int passedStatus )
{
    fout << testName << ": ";
    if(passedStatus == 2)
    {
        fout << "Passed with presentation errors" << std::endl;
    }
    else if(passedStatus == 1)
    {
        fout << "Passed" << std::endl;
    }
    else
    {
        fout << "Failed" << std::endl;
    }
    
    return;
}

/*******************************************************************************
 * @Function RunTestCase
 * @Author Andrew Koc (Note some code was taken from find_tst written by
 *                       Colter Assman and Shaun Gruenig)
 * @Author Adam Meaney
 * 
 * @Description:
 * This function will take a given .tst file, generate the names for the 
 * corresponding .out and .ans files and then run a specified executable with
 * the .tst file as the input and the .out file for the output.  Note the .out
 * file will be stored in the a different directory than the one the .tst file
 * will be.
 *
 * @param[in] exec - the full path to the executable to be run
 * @param[in] test_case - the name of the .tst file to be tested
 * @param[in] curr_dir - the directory in which the .tst file is stored
 * @param[in] student_dir - the directory of the student whose program is being tested
 * @param[out] rec - the student record for the given executable
 * @param[out] log - the log file for given executable
 *
 * @returns true - the executable passed the test case
 * @returns false - the executable failed the test case
 *
*******************************************************************************/
bool RunTestCase(string exec, string test_case, string curr_dir, 
	string student_dir, data_struct *rec, ofstream &log)
{
	int passed;
	string outfilename, ansfilename, testname, command;
	int pid = 0;
	int wait_pid;
	int status;
	bool finished = false;
	
	
	//increment number of tests found/ran
	rec->total++;

    //discards file extension from .tst file
    testname = test_case.substr ( 0, test_case.length() - 4 );

    //creates name for .out file, including the full path
    outfilename = student_dir + '/' + testname + ".out";

    //creates name for .ans file, including the full path
    ansfilename = curr_dir + "/" + testname + ".ans";

    //creates command string to run the .tst file
    command = exec + " < " + curr_dir
                + "/" + test_case + " > " + outfilename
                + " 2>/dev/null";

    //run command to test program with current .tst file
    pid = fork();
    if (pid < 0)
    {
        cout << "An error occurred while attempting to run student in " << student_dir << endl;
        exit(-2);
    }
    else if (pid == 0)
    {
        // Child
        //system ( command.c_str() );
        execl("/bin/sh", "/bin/sh", "-c", command.c_str(), NULL);
        exit(5);
    }
    else
    {
        // in parent, check for child exit
        int timer = 0;

        while (true)
        {
            // sleep one second and see if process is done
            sleep (1);
            timer++;
            wait_pid = waitpid(pid, &status, WNOHANG);

            // if process is done, break out of loop
            if (wait_pid != 0)
            {
                finished = true;
                break;
            }

            // if time limit exceeded, kill child process
            if (timer >= MAX_TIMEOUT)
            {
                rec -> crit_failed = true;
                finished = false;
                kill(pid, 9);
            }
        }
    }
    
    system(string("pkill -f " + exec).c_str());

	//determine if the program passed
	passed = run_diff(ansfilename, outfilename);

	StudentLogWrite(log, testname, passed);
	
	//The program passed the test
	if( passed && finished == true )
	{
		rec->passed++;
	}
	//The program did not pass the test
	else
	{
		rec->failed++;
		
		//determine if this was a crit test it failed
		//If the .find function finds it it will return the
		//starting position of the string, so if it doesn't
		//equal the npos, or null position, if was found
		if( stringEndsWith( test_case, "_crit.tst" ) )
		{
			rec->crit_failed = true;
		}
	}

	return passed;
}

void run_gprof(string progName, ofstream &log)
{
    string command, gprofFile;
    string funcName, percent, temp;
    ifstream fin;
    int i;
    
    //run gprof
    gprofFile = progName + ".gprof";
    command = "gprof " + progName + " > " + gprofFile;
    system( command.c_str() );
    
    
    //parse out function names and percentages from .gprof file
    fin.open( gprofFile.c_str() );
    
    //first 7 lines are header stuff
    for(i = 0; i < 7; i++)
    {
        getline(fin, temp);
    }
    
    log << "gprof results: " << endl;
    
    while( fin >> percent )
    {
        //don't want next 5 things
        for(i = 0; i < 5; i++)
        {
            fin >> funcName;    
        }
        
        log << funcName << " : " << percent << "%" << endl;
    }
    
    fin.close();
}

/*******************************************************************************
 * @Function run_gcov
 * @Author Adam Meaney
 * 
 * @Description:
 * This function will take a given log file and write the code coverage found
 * by gcov for the student for whom that log exists.
 *
 * @param[in] progName - the name of the program that was run
 * @param[in] log - the log file to write to
 *
*******************************************************************************/
void run_gcov(string progName, ofstream &log)
{
    string command = "";
    char buffer[40] = {'\0'};
    ifstream fin;

    command += "gcov " + progName + " | grep \"Lines executed\" > gcovOutfile.g ";

    system( command.c_str() ); 

    fin.open("gcovOutfile.g");

    fin.getline(buffer, 40);

    command = buffer;

    if (command.length() == 0)
        log << "Coverage unable to be checked." << endl;
    else
    {
        command = command.substr(0, command.find("%", 0) + 1);
        log << command << endl;
    }
        
    fin.close();
    
    remove("gcovOutfile.g");
   
}


/*******************************************************************************
 * @Function studentDirCrawl
 * @Author: Jonathan Tomes
 * 
 * @Description:
 * This function will create a class log file in the current working directory
 * and then search through the directory and find each student directory, 
 * ignoring any directory with the word "test" in it. 
 * It will then change into each student directory it finds. 
 * There it will compile the source code and create a log file. Then it will
 * call the testing function to test each student's program, passing it the
 * student's name, path to program, and streams for the created log file.
*******************************************************************************/

void studentDirCrawl( string rootDir )
{
    string classLogName;
    ofstream classLog;
    string studentLogName;
    ofstream studentLog;
    DIR * dir = opendir( rootDir.c_str() );
    struct dirent* file;                //file entity struct from dirent.h
    string filename;
    string studentDir;
    string studentName;
    time_t timer;                       //a time object from time.h
    data_struct rec;
    
    //a string to describe what to name the program
    //and where to compile it too.
    string progName = rootDir + "/a.out";
    
    //first create a class log file to store the final results for each
    //student.
    //start with getting a timer to time stamp the file name.
    
    time( &timer );
    classLogName = "Class_";
    classLogName += get_time();
    //classLogName += ctime( &timer );
    classLogName += ".log";
    classLog.open( classLogName.c_str() );
    
    //start crawling through the directory.
    while( ( file = readdir(dir ) ) != NULL )
    {
        //get the file name.
        filename = file ->d_name;
        //skip over "." and "..." and any with "test" in the name.
        if( filename != "." && filename != ".." && 
                (filename.find( "test") == string::npos ) )
        {
            //Check to see if it is a directory
            //a d_type of 4 is what the value should be.
            if( (int) file->d_type == 4 )
            {
                //get the student name and directory.
                studentName = filename;
                studentDir = rootDir + '/' + filename;
                
                
                //change into the student directory.
                chdir( filename.c_str() );
                
                //open the student log for creation.
                //and time stamp the name so to differentiate it
                //from other testing rounds.
                studentLogName = studentName;
                studentLogName += "_";
                studentLogName += get_time();
                //studentLogName += ctime( &timer );
                studentLogName += ".log";
                studentLog.open( studentLogName.c_str() );
                
                //compile the program in this directory
                //into the root directory.
                compile( progName );

                //Clear record
                rec.failed = 0;
                rec.crit_failed = false;
                rec.passed = 0;
                rec.total = 0;
                
                //call test function to find and run tests
                testCrawl( rootDir + '/' + "test", progName, studentLog, &rec, studentDir );

                
                //Final log write and write to class log
                FinalLogWrite(studentLog, studentName, &rec);
                FinalLogWrite(classLog, studentName, &rec);
                
                //determine code coverage and performance, append to log
	            run_gprof(studentName, studentLog);
                
                run_gcov(studentName, studentLog);
                
                studentLog.close();

                chdir( rootDir.c_str() );
                
            }
        }
    }
    
    //close the class log.
    classLog.close();
    return;
}

/******************************************************************************
 * @Fuction testCrawl
 * @author Erik Hattervig
 * 
 * Description:
 * A recursive function that traverses the Test directory that is provided.
 * It calls will call the testing function when a .tst file is found in the
 * directory, and pass on the rec struct for the over all student final pass
 * and fails.
 *
 * @param[in] testPath - a string of the path to the directory that is being
 *                       processed
 * @param[in] exePath - a string of the path to the executable being tested
 * @param[in] studentLog - A file stream to the log file that the test data
 *                         will be written to
 * @param[in] rec - a struct containing the total cases, passed cases, and
 *                  a bool for crit test fail
 * @param[in] studentPath - A path the the student directory
 *
 *****************************************************************************/
void testCrawl( string testPath, string exePath, ofstream &studentLog, 
  data_struct *rec, string studentPath )
{
    DIR* dir = opendir( testPath.c_str() );  // Open the current level of the
                                           // traversal
    struct dirent* file;  // File entry structure from dirent.h
    string filename;      // used in getting file names
  
    // Read each file
    // Readdir returns next file in the directory and returns null if no other
    //   files exist
    while ( ( file = readdir(dir) ) != NULL )
    {
        // place file name into string filename for easier checking
        filename = file->d_name;
    
        // skip over the directories "." and ".."
        if ( filename != "." && filename != ".." )
        {
            // checks if the file is a subdirectory, 4 is the integer identity
            //   for the dirent struct on Lixux systems
            if ( (int) file->d_type == 4 )
            {
                // move into the sub-directory
                testCrawl( testPath + '/' + filename, exePath, studentLog, rec , studentPath );
            }
            else
            {
                // check if the file has a .tst in it. String find returns 
                //   string::npos if the substring cannot be found
                if ( stringEndsWith(filename, ".tst" ) )
                {
                    // pass the file onto the grader
                    RunTestCase( exePath, filename, testPath, studentPath, rec, studentLog );
                }
            }
        } 
    }
}

/******************************************************************************
 * @Fuction askForTimeout
 * @author Adam Meaney
 * 
 * Description:
 * This function prompts to change the timeout on the tests. It takes in a y
 * for yes and an n for no. If yes, then it prompts to enter the new timeout
 * and changes the global timeout variable.
 *
 *****************************************************************************/
void askForTimeout()
{
    char choice;
    cout << "Do you want to change the timeout (Default: 60) y/n? ";
    cin >> choice;
    
    if (choice == 'y' || choice == 'Y')
    {
        cout << "Timeout: ";
        cin >> MAX_TIMEOUT;
    }
    
}

/******************************************************************************
 * @Fuction manualDiff
 * @author Adam Meaney
 * 
 * Description:
 * This function checks for differences between 2 files, after a normal diff
 * has been run. Returns true if pass, false if fails diff.
 *
 *****************************************************************************/
bool manualDiff(string file1, string file2)
{
    string ansWord = "";
    string outWord = "";
    
    ifstream ansFin;
    ifstream outFin;
    bool success = true;
    
    ansFin.open(file1.c_str());
    outFin.open(file2.c_str());
    
    if ( !ansFin || !outFin )
    {
        cout << "Catastrophic error, file disappeared during testing." << endl;
        ansFin.close();
        outFin.close();
        exit(-5);
    }
    
    while(ansFin >> ansWord)
    {
        if (!(outFin >> outWord))
        {
            success = false;
            break;
        }
        if (ansWord != outWord)
        {
            //run the tests, break if they fail.
            if (testResults(ansWord, outWord) == false)
            {
                success = false;
                break;
            }
        }
    }
    
    //Check for additional words in outFin
    
    if ((outFin >> outWord) && success == true)
    {
            success = false;
    }
    
    return success;
}


/******************************************************************************
 * @Fuction testResults
 * @author Adam Meaney
 * 
 * Description:
 * This function takes 2 words and checks if they are the same word but in the
 * incorrect order, 2 words that start and end the same, or a number that is 
 * the same as the ansWord if rounded.
 *
 * param[in] ansWord - the word from the .ans file
 * param[in] outWord - the word from the .out file
 * returns success - if the words are ruled the same
 *
 *****************************************************************************/
bool testResults(string ansWord, string outWord)
{
    int ansLength = ansWord.length();
    int outLength = outWord.length();
    int precision;
    double ansDub;
    double outDub;
    
    istringstream ansStream;
    istringstream outStream;
    
    ansStream.str(ansWord);
    outStream.str(outWord);
    
    precision = ansWord.find(".", 0);
    
    if (precision > 0)
    {
        precision = ansLength - (precision + 1);
        if ( (ansStream >> ansDub) && (outStream >> outDub) )
        {       
            ansDub *= pow(10, precision);
            outDub *= pow(10, precision);
            
            if (int(ansDub) == int(outDub + .50000005))
                return true;
            else
                return false;
            
        }
    }
    
    if (ansWord[0] == outWord[0] && ansWord[ansLength - 1] == outWord[outLength - 1])
        return true;
        
    sort(ansWord.begin(), ansWord.end());
    sort(outWord.begin(), outWord.end());
    
    if (ansWord == outWord)
        return true;
    
    return false;
}
