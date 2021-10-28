package edu.cs300;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.Enumeration;
import java.util.Scanner;
import java.util.Vector;
import java.util.ArrayList;

public class FileReaderThread implements Runnable
{
	/// Helper class to hold column attribute data
	class ColumnAttributes
	{
		public int startValue;
		public int endValue;
		public String name;
	};

	/// Constructor
	public FileReaderThread(int id, String filepath)
	{
		_id = id;
		_filepath = filepath;
	}

	/// Method called by thread
	public void run()
	{
		/// Init file and scanner
		File report= new File(_filepath);
		Scanner reportScanner = new RecordScanner(report);

		/// Load report attributes
		String reportTitle = reportScanner.nextLine();
		String reportSearchString = reportScanner.nextLine();
		String outputFileName = reportScanner.nextLine();

		/// Debug info
		DebugLog.log("Thread id " + String.toString(_id) + " loaded report title: " + reportTitle);
		DebugLog.log("Thread id " + String.toString(_id) + " loaded report search string: " + reportTitle);
		DebugLog.log("Thread id " + String.toString(_id) + " loaded output file name: " + reportTitle);

	}

	string _filepath = "";
	int _id = -1;
}

public class ReportingSystem
{
	public ReportingSystem()
	{
	  DebugLog.log("Starting Reporting System");
	}


	public int loadReportJobs() {
		int reportCounter = 0;
		try {

			   File file = new File("report_list.txt");
			   Scanner reportList = new Scanner(file);
			   int numFiles = Integer.parseInt(reportList.nextLine());

			   for(int i = 1; i <= numFiles; i++)
			   {
				   String fileName = reportList.nextLine();

				   new Thread
			   }


 		     //load specs and create threads for each report
				 DebugLog.log("Load specs and create threads for each report\nStart thread to request, process and print reports");

			   reportList.close();
		} catch (FileNotFoundException ex) {
			  System.out.println("FileNotFoundException triggered:"+ex.getMessage());
		}
		return reportCounter;

	}

	public static void main(String[] args) throws FileNotFoundException {


		   ReportingSystem reportSystem= new ReportingSystem();
		   int reportCount = reportSystem.loadReportJobs();


	}

}
