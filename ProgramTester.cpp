/******************************************************************************
 *
 *  Simple Program Tester
 *
 *  Authors: Colter Assman, Samuel Carroll, Shaun Gruenig
 *  Adventure Line Authors: Erik Hattervig, Andrew Koc, Jonathan Tomes
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


/////////includes//////////////////////////////////////////////////////////////
#include <iostream>
#include <stdlib.h>
#include <cstdlib>
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <fstream>
#include <ctime>

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
string get_time ();
void menuLoop( string rootDir );
bool run_diff ( string file1, string file2 );
bool RunTestCase(string exec, string test_case, string curr_dir, 
	string student_dir, data_struct *rec, ofstream &log);
void StudentLogWrite( std::ofstream &fout, string testName, bool passedStatus );
void studentDirCrawl( string rootDir );
void testCrawl( string testPath, string exePath, ofstream &studentLog, 
  data_struct *rec, string studentPath );

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
            if( filename.find(".cpp") != string::npos )
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
    
    //check to see if a program name was specified.
    if( !progName.empty() )
    {
        command = "g++ -o " + progName + " " + cppFile;
    }
    else
    {
        command = "g++ " + cppFile;
    }
    
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
 * 
 * @Description:
 * Asks the user for the type of test data, the number of test cases to
 * generate, and the number of arguments in each test case, it then generates
 * the test cases in a GeneratedTests folder and then call for the test cases
 * for run through the golden program.
 *
 * @parm[in] rootDir - the path to the root directory
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
        cout << "\nType of test data?\n  1: Integer\n  2: Float\n";
        cout << "Selection: ";
        
        cin >> inputType;
        
        if( inputType != "1" && inputType != "2" )
        {
            cout << "Please enter a valid option" << endl;
        }
    
    }while( inputType != "1" && inputType != "2" );
    
    do
    {
        // Print menu for number of test cases created
        cout << "\nWhat number of test cases would you like to " <<
            "generate?\n";
        
        cin >> inputNumber;
        // Make sure the input is a number
        if ( inputNumber.find_first_not_of("0123456789") != 
            string::npos )
        {
            cout << "Please enter a valid number:\n";
        }
    }while( inputNumber.find_first_not_of("0123456789") != 
        string::npos );
    
    do
    {
        // Print Menu for number of arguments in each test case
        cout << "\nWhat number of arguments would you like in each " <<
        "test case?\n";
        cin >> inputArgs;
        
        if ( inputArgs.find_first_not_of("0123456789") != 
            string::npos )
        {
            cout << "Please enter a valid number:\n";
        }
        
    }while( inputArgs.find_first_not_of("0123456789") != 
        string::npos );
    
    // change into the test folder
    testDir = rootDir + "/test";
    chdir( testDir.c_str() );
    
    // delete the generated test in the test folder
    system( "rm -rf GeneratedTests/" );
    // recreate generated tests folder and change into it.
    system( "mkdir GeneratedTests" );
    testDir = testDir + "/GeneratedTests";
    chdir( testDir.c_str() );
    
    // convert input to integers
    numberOfTests = atoi( inputNumber.c_str() );
    numberOfArgs = atoi( inputArgs.c_str() );
    
    
    if( inputType == "1" )
    {
        // generate test cases for integers
        
        for( i = 0 ; i < numberOfTests ; i++ )
        {
            sprintf( buffer, "%d", i);
            filename = "Test_" + (string)buffer;
            filename += ".tst";
            fout.open( filename.c_str() );
            
            for( j = 0 ; j < numberOfArgs ; j++ )
            {
                // generate ints, maxed at 1000
                fout << rand() % 1000;
                fout << endl;
            }
            fout.close();
        }
        
    }
    else if ( inputType == "2" )
    {
        // generate test cases for floats
        
        for( i = 0 ; i < numberOfTests ; i++ )
        {
            sprintf( buffer, "%d", i);
            filename = "Test_" + (string)buffer;
            filename += ".tst";
            fout.open( filename.c_str() );
            
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
        filename = "touch Test_" + (string)buffer;
        filename += ".ans";
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
 * This function promps the user for a option, its options will include running
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
            // run the program as normal
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
 *
******************************************************************************/

bool run_diff ( string file1, string file2 )
{
    string command; //string that will hold the command
    int result; // result of the diff system function

    command = "diff "; // start by writing the diff name and space
    command += file1; // first file we want to check
    command += " "; // space between the files
    command += file2; // second file we want to check
    command += " > /dev/null"; // stops the diff function from writing to the
    // console

    result = system ( command.c_str ( ) ); // run the command function and save
    // the result

    if ( !result ) // if we get a zero there is no difference between the files
        return true; // return true so we know the test passed

    else // if we don't get a zero there was a diffence between the files
        return false; // return false if the test failed
} // end of run_diff function

/******************************************************************************
 * @Function: StudentLogWrite
 * @Author: Jonathan Tomes
 *
 * @Description:
 *  Prints the status of a test to the file stream. Status should be weather
 * or not the a test passed (true) or failed (false).
 *
 * @param[in] fout - the stream to write to.
 * @param[in] testName - name of the test.
 * @param[in] passedStatus - true - test passed.
 *                          false - test failed.
******************************************************************************/

void StudentLogWrite( std::ofstream &fout, string testName, bool passedStatus )
{
    fout << testName << ": ";
    if(passedStatus)
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
	bool passed;
	string outfilename, ansfilename, testname, command;
	
	//increment number of tests found/ran
	rec->total++;

    //discards file extension from .tst file
    testname = test_case.substr ( 0, test_case.length() - 4 );

    //creates name for .out file, including the full path
    outfilename = student_dir + '/' + testname + ".out";

    //creates name for .ans file, including the full path
    ansfilename = curr_dir + "/" +
                    testname + ".ans";

    //creates command string to run the .tst file
    command = exec + " < " + curr_dir
                + "/" + test_case + " > " + outfilename
                + " 2>/dev/null";

    //run command to test program with current .tst file
    system ( command.c_str() );

	//determine if the program passed
	passed = run_diff(outfilename, ansfilename);

	StudentLogWrite(log, testname, passed);
	
	//The program passed the test
	if( passed )
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
		if( test_case.find("_crit") != string::npos )
		{
			rec->crit_failed = true;
		}
	}

	return passed;
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
                testCrawl( rootDir + '/' + "test", progName, studentLog,
                  &rec, studentDir );

                //Final log write and write to class log
                FinalLogWrite(studentLog, studentName, &rec);
                FinalLogWrite(classLog, studentName, &rec);
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
                testCrawl( testPath + '/' + filename, exePath,
                studentLog, rec , studentPath );
            }
        else
        {
            // check if the file has a .tst in it. String find returns 
            //   string::nops if the substring cannot be found
            if ( filename.find( ".tst" ) != string::npos )
            {
                // pass the file onto the grader
                RunTestCase( exePath, filename, testPath, studentPath, rec,
		        studentLog);
            }
      }
    } 
  }
  
  return;
}
