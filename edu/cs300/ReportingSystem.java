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

	public void loadReportJobs()
	{
		/// Create variables for thread
		Integer reportCounter = 0;
		ArrayList<ReportGeneratorThread> fileReaders = new ArrayList<ReportGeneratorThread>();

		// Why is this here?
		DebugLog.log("Load specs and create threads for each report\nStart thread to request, process and print reports");

		try
		{
			File file = new File("report_list.txt");
			Scanner reportList = new Scanner(file);
			reportCounter = Integer.parseInt(reportList.nextLine());

			/// Create and start threads
			for(Integer i = 1; i <= reportCounter; i++)
			{
			   String fileName = reportList.nextLine();
			   Debug("starting thread #" + i.toString() + " with filepath " + fileName);

			   ReportGeneratorThread newThread = new ReportGeneratorThread(i, reportCounter, fileName);
			   fileReaders.add(newThread);

			   /// Start thread
			   newThread.start();
			}
			// Close file
			reportList.close();

			Debug("Created " + reportCounter + " threads successfully!");

			// Join threads when complete
			for(ReportGeneratorThread thread : fileReaders)
			{
				thread.join();
			}
		}
		catch(FileNotFoundException ex)
		{
			  System.out.println("FileNotFoundException triggered:" + ex.getMessage());
		}
		catch(InterruptedException ex)
		{
			System.out.println("InterruptedException triggered:" + ex.getMessage());
		}
	}

	private void Debug(String message)
	{
		DebugLog.log("ReportingSystem: " + message);
	}

	public static void main(String[] args) throws FileNotFoundException
	{
		   ReportingSystem reportSystem= new ReportingSystem();
		   reportSystem.loadReportJobs();
	}
}
