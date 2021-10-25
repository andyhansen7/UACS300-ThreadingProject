package edu.cs300;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.Enumeration;
import java.util.Scanner;
import java.util.Vector;

public class ReportingSystem {


	public ReportingSystem() {
	  DebugLog.log("Starting Reporting System");
	}

	public int loadReportJobs() {
		int reportCounter = 0;
		try {

			   File file = new File ("report_list.txt");

			   Scanner reportList = new Scanner(file);

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
