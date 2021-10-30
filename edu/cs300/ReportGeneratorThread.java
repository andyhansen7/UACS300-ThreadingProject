package edu.cs300;

import java.io.*;
import java.util.Scanner;
import java.util.ArrayList;

public class ReportGeneratorThread extends Thread
{
    /// Class member variables
    String _filepath;
    Integer _id;
    Integer _numReports;

    /// Constructor
    public ReportGeneratorThread(Integer id, Integer numReports, String filepath)
    {
        this._id = id;
        this._numReports = numReports;
        this._filepath = filepath;
    }

    /// Method called by thread
    public void run()
    {
        /// Init file and scanner
        File report= new File(_filepath);
        Scanner reportScanner = null;

        try
        {
            reportScanner = new Scanner(report);
        }
        catch (FileNotFoundException ex)
        {
            System.out.println("FileNotFoundException triggered:" + ex.getMessage());
        }

        /// Load report attributes
        String reportTitle = reportScanner.nextLine();
        String reportSearchString = reportScanner.nextLine();
        String outputFileName = reportScanner.nextLine();

        /// Debug info
        Debug("loaded report title: " + reportTitle);
        Debug("loaded report search string: " + reportSearchString);
        Debug("loaded output file name: " + outputFileName);

        /// Load column data
        ArrayList<ColumnAttributes> columnAttributes = new ArrayList<ColumnAttributes>();
        int index = 0;
        while(reportScanner.hasNextLine())
        {
            String columnData = reportScanner.nextLine();
            if(columnData.length() == 0) break;

            /// Split string into components
            String[] startValueSplit = columnData.split("-");
            String[] endValueSplit = startValueSplit[1].split(",");

            /// Build helper object
            ColumnAttributes columnAttribute = new ColumnAttributes();
            columnAttribute.startValue = Integer.parseInt(startValueSplit[0]);
            columnAttribute.endValue = Integer.parseInt(endValueSplit[0]);
            columnAttribute.name = endValueSplit[1];

            /// Debug info
            Debug("loaded column " + columnAttribute.name + ", start " + columnAttribute.startValue.toString() + ", end " + columnAttribute.endValue.toString());

            /// Load array
            columnAttributes.add(columnAttribute);
            index++;
        }

        /// Send request on System V queue
        Debug("sending query request to C application...");
        MessageJNI.writeReportRequest(_id, _numReports, reportSearchString);

        try
        {
            File outputReport = new File(outputFileName);
            FileWriter outputWriter = new FileWriter(outputReport);

            /// Write headers and title
            outputWriter.write(reportTitle + "\n");
            for(ColumnAttributes attr : columnAttributes)
            {
                outputWriter.write(attr.name + "\t");
            }
            outputWriter.write("\n");

            /// Create output file
            while (true)
            {
                // Get response from C application
                String queryResponse = MessageJNI.readReportRecord(_id);
                if (queryResponse.length() == 0) break;

                /// Write data from queue
                for (ColumnAttributes attr : columnAttributes) {
                    /// Create field using column attributes
                    String field = queryResponse.substring(attr.startValue - 1, attr.endValue);
                    outputWriter.write(field + "\t");
                }
                outputWriter.write("\n");
            }

            // Close writer
            outputWriter.close();
        }
        catch(IOException ex)
        {
            System.out.println("IOException triggered:" + ex.getMessage());
        }

        /// Close scanner
        reportScanner.close();
    }

    private void Debug(String message)
    {
        DebugLog.log("Thread #" + _id + ": " + message);
    }
}