/******************************************************************************
 *
 *  Simple Program Tester
 *
 *  Authors: Colter Assman, Samuel Carroll, Shaun Gruenig
 * CS470 - Software Engineering
 * Dr. Logar
 * February 19
 *
 * Program Description:
 *     This program is to be placed into a directory containing
 *     subdirectories with .cpp files, associated .tst files which
 *     contain test input for the program contained in the cpp file,
 *     and .ans files that contain the expected output associated with
 *     said input. This program will then find the specified cpp file,
 *   compile and run the program against all given test cases, and
 *    output a log file in the same directory as the .cpp file.
 *
 *     A more in-depth description of this program can be found in
 *     the accompanying documentation.
 *
 *
******************************************************************************/


/////////includes//////////////////////////////////////////////////////////////
#include <iostream>
#include <stdlib.h>
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
};


///////////function prototypes/////////////////////////////////////////////////
void find_tst ( string root, string curr_dir, string target, ofstream &fout,
                data_struct* test_stats );
string get_time();
void parse_directory ( string root, int level, string target, ofstream &fout,
                       data_struct* test_stats );
bool run_diff ( string file1, string file2 );

void FinalLogWrite( std::ofstream &fout, string name, int numPassed, 
        int numTotal);

void StudentLogWrite( std::ofstream &fout, string testName, bool passedStatus );

bool compile( string progName);
void studentDirCrawl( string rootDir );




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
int main ( int argc, char* argv[] )
{
    ofstream fout;            // file stream for .log file
    string masterDir;         // used to hold the directory by the user
    data_struct test_stats;   // used to hold results of tests (# passed, etc)
    int percent;              // used to hold percent passed & percent failed

    // initialize values in test_stats struct
    test_stats.passed = 0;
    test_stats.failed = 0;
    test_stats.total = 0;

    // get users argument
    masterDir = argv[1];

    // locates target directory and calls functions necessary to run
    // the applicable tests and gather the data for the log file
    parse_directory ( get_current_dir_name() , 1, masterDir, fout, &test_stats );


    // this block outputs all the collected statistics to the log file
    fout << endl;
    fout << "Total Passed: " << test_stats.passed << endl;
    fout << "Total Failed: " << test_stats.failed << endl;
    percent = ( int ) ( ( ( double ) test_stats.passed ) / test_stats.total
                        * 100 + 0.5 );
    fout << "Percent Passed: " << percent << "%" << endl;
    percent = ( int ) ( ( ( double ) test_stats.failed ) / test_stats.total
                        * 100 + 0.5 );
    fout << "Percent Failed: " << percent << "%" << endl;
    fout.close();
    return 0;
}



/******************************************************************************
 *
 *  parse_directory function
 *
 *  @Description:
 *  This function searches through the directory and sub-directories that
 *  the program was run from until it finds a .cpp file that has
 *  a file name matching the one specified by the user. It then compiles
 *  the .cpp file, opens a related log file and calls the find_tst
 *  function to locate the tests associated with that target program
 *
 *  Parameters:
 *      string root - contains name of current root directory
 *      string target - contains the file name specified by the user
 *      ofstream &fout - file stream for writting to log file
 *      data_struct* test_stats - contains pointer to a structure
 *          which contains collected statistics from tests
 *
 *  @Authors: Colter Assman, Shaun Gruenig
 *
******************************************************************************/
void parse_directory ( string root, int level, string target, ofstream &fout,
                       data_struct* test_stats )
{
    string temp, curr_dir, command, logfilename;
    unsigned char test;
    DIR *dir = opendir ( root.c_str() ); // open the current directory
    struct dirent *entry;

    if ( !dir ) // not a directory
        return;

    //this while loop continues to recursively parse a given directory
    //while there are still unchecked files in it, until it locates
    //the cpp file with the base name specified by the user
    while ( entry = readdir ( dir ) ) // notice the single '='
    {
        temp = entry->d_name; //get the file/directory name of current file

        if ( level == 2 ) //if it is the level of the cpp file
            curr_dir = root; //save path of cpp file for later use


        //this if statement checks to if the item in temp is a special system
        //file and ignores it if it is
        if (  temp != "." && temp != ".." && temp[temp.size() - 1] != '~'  )
        {
	    //store length of the current file's name
            int length = temp.length();

            //this if block checks if the current file ends with ".cpp"
            if ( length > 4 && temp[length - 1] == 'p' 
			    && temp[length - 2] == 'p' 
			    && temp[length - 3] == 'c' 
			    && temp[length - 4] == '.'
                   	    && level != 1 )
            {
                //this if statement checks if the cpp file is the
                //one specified by the user
                if ( temp.substr ( 0, temp.length() - 4 ) == target )
                {
                    //creates command string to compile .cpp file
                    command = "g++ -o " + curr_dir + "/" + target
                              + " " + curr_dir + "/" + temp;
                    system ( command.c_str() ); //compiles .cpp file

                    //creates name of log file using name specified 
		    //by user, date, and time
                    logfilename = curr_dir + "/"  + target + "_" + get_time() 
					   + ".log";
                    fout.open ( logfilename.c_str() );

		    //sends the ccp directory to find tst
                    find_tst ( curr_dir, curr_dir, target, fout, test_stats ); 
                }
            }
            //attempt to look in directory; recursive call will return 
	    //immediately if the item in temp is not a directory
            else
            {
                parse_directory ( root + '/' + temp, level + 1, target, 
				  fout, test_stats );
            }
        }
    }


    closedir ( dir );

}

/******************************************************************************
 *
 *  find_tst function
 *
 *  @Description:
 *  This function searches through the directory and sub-directories that
 *  the program was run from until it finds a .cpp file that has
 *  a file name matching the one specified by the user. It then compiles
 *  the .cpp file, opens a related log file and calls the find_tst
 *  function to locate the tests associated with that target program
 *
 *  Parameters:
 *      string root - contains name of directory containing the
 *          the program we are testing
 *      string curr_dir - contains name of current directory
 *      string target - contains the file name specified by the user
 *      ofstream &fout - file stream for writting to log file
 *      data_struct* test_stats - contains pointer to a structure
 *          which contains collected statistics from tests
 *
 *  @Authors: Colter Assman, Shaun Gruenig
 *
******************************************************************************/
void find_tst ( string root, string curr_dir, string target, ofstream &fout, 
		data_struct* test_stats )
{
    string temp, command, testname, outfilename, ansfilename;
    unsigned char test;
    DIR *dir = opendir ( curr_dir.c_str() ); // open the current directory
    struct dirent *entry;

    if ( !dir )
        return;


    while ( entry = readdir ( dir ) ) // notice the single '='
    {
        temp = entry->d_name;

        //this if statement checks to if the item in temp is a special system
        //file and ignores it if it is
        if ( temp != "." && temp != ".." && temp[temp.size() - 1] != '~' )

        {
            int length = temp.length();

            //this if statement checks if the current file ends in .tst
            if ( length > 4 && temp[length - 1] == 't' 
			    && temp[length - 2] == 's'       
			    && temp[length - 3] == 't' 
			    && temp[length - 4] == '.' )
            {
                test_stats->total++; //increment number of tests found/ran

                //change to correct directory to run the program to be tested
                chdir ( target.c_str() );

                //discards file extension from .tst file
                testname = temp.substr ( 0, length - 4 );

                //creates name for .out file, including the full path
                outfilename = curr_dir + "/" +
                              testname + ".out";

                //creates name for .ans file, including the full path
                ansfilename = curr_dir + "/" +
                              testname + ".ans";

                //creates command string to run the .tst file
                command = "./" + target + " < " + curr_dir
                          + "/" + temp + " > " + outfilename
                          + " 2>/dev/null";

                //run command to test program with current .tst file
                system ( command.c_str() );
                fout << testname << ": ";

                //check if output file matches the expected output and
		//increment appropriate statistic
                if ( run_diff ( outfilename, ansfilename ) )
                {
                    fout << "Passed" << endl;
                    test_stats->passed++;
                }
                else
                {
                    fout << "Failed" << endl;
                    test_stats->failed++;
                }

                //remove .out file
                command = "rm " + outfilename;
                system ( command.c_str() );
            }
            else
            {
                find_tst ( root, curr_dir + '/' + temp, target, fout, 
				test_stats );
            }
        }
    }



    closedir ( dir );

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

void FinalLogWrite( std::ofstream &fout, string name, int numPassed, 
        int numTotal )
{
    //calculate the percentage passed.
    float perPassed;
    perPassed = numPassed/numTotal;
    
    //check to see if they passed crit Tests
    if( numPassed > 0)
    {
        //Prints student name and percentage passed.
        fout << name << ": " << perPassed << "% passed" << std::endl;
    }
    else
    {
        //They failed!
        fout << name << ": "<< "FAILED" << std::endl;
    }
    return;
}

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
    struct dirent* file;
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
 * 
*******************************************************************************/

void studentDirCrawl( string rootDir )
{
    string classLogName;
    ofstream classLog;
    string studentLogName;
    ofstream studentLog;
    DIR * dir = opendir( rootDir.c_str() );
    struct dirent* file;
    string filename;
    string studentDir;
    string studentName;
    time_t timer;
    
    //first create a class log file to store the final results for each
    //student.
    //start with getting a timer.
    time( &timer );
    classLogName = "Class_";
    classLogName += ctime( &timer );
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
            if( (int) file->d_type == 4 )
            {
                //get the student name and directory.
                studentName = filename;
                studentDir = rootDir+ filename;
                
                //cout << studentDir << endl;
                //change into the student directory.
                chdir( filename.c_str() );
                
                //open the student log for creation.
                studentLogName = studentName;
                studentLogName += "_";
                studentLogName += ctime( &timer );
                studentLogName += ".log";
                studentLog.open( studentLogName.c_str() );
                
                compile( "" );
                //call test function some where around here
                studentLog.close();
                chdir( rootDir.c_str() );
                
            }
        }
    }
    
    //close the class log.
    classLog.close();
    return;
}
