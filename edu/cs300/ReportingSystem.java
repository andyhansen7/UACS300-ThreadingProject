package edu.cs300;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.Enumeration;
import java.util.Scanner;
import java.util.Vector;
import java.util.ArrayList;

public class ReportingSystem
{
	public ReportingSystem()
	{
	  DebugLog.log("Starting Reporting System");
	}

	public Integer loadReportJobs()
	{
		/// Create variables for thread
		Integer reportCounter = 0;
		ArrayList<ReportGeneratorThread> fileReaders = new ArrayList<ReportGeneratorThread>();

		try
		{
			   File file = new File("report_list.txt");
			   Scanner reportList = new Scanner(file);
			   reportCounter = Integer.parseInt(reportList.nextLine());

			   /// Create and start threads
			   for(Integer i = 1; i <= reportCounter; i++)
			   {
				   String fileName = reportList.nextLine();

				   DebugLog.log("loadReportJobs: Starting thread #" + i.toString() + " with filepath " + fileName);

				   ReportGeneratorThread newThread = new ReportGeneratorThread(i, fileName);
				   fileReaders.add(newThread);

				   /// Start thread
				   newThread.start();
			   }

			   reportList.close();
		}
		catch(FileNotFoundException ex)
		{
			  System.out.println("FileNotFoundException triggered:" + ex.getMessage());
		}

		return reportCounter;
	}

	public static void main(String[] args) throws FileNotFoundException
	{
		   ReportingSystem reportSystem= new ReportingSystem();
		   Integer reportCount = reportSystem.loadReportJobs();
	}
}
