#include "Utilities.h"
#include "LogInfo.h"

namespace Utilities
{
    /**
     * Read the text in from a file stored in flash
     * 
     * @param fileName The name of the file
     * @return The complete file as a string
     */
    String readFile(const char *fileName)
    {
        File file = openFile(fileName);
        String debugLogData;
        if (!file)
        {
            return debugLogData;
        }

        while (file.available())
        {
            debugLogData += char(file.read());
        }
        //debugLogData += '\0';
        file.close();
        return debugLogData;
    }

    /**
     * Read the text in from a file stored in flash and place the contents in the buffer
     * 
     * @param fileName The name of the file
     * @param buffer The buffer to hold the contents of the file
     * @param size The size of the buffer
     * @return The actual size of the file read
     */
    size_t readFile(const char *fileName, char *buffer, size_t size)
    {
        size_t result = 0;
        if (SPIFFS.exists(fileName))
        {
            File file = openFile(fileName);
            if (file)
            {
                result = file.read((uint8_t *)buffer, size);
                file.close();
                //buffer[size] = '\0';
            }
        }
        LogInfo.log(LOG_VERBOSE, "File %s - Expected Size %i Actual Size %i", fileName, size, result);
        return result;
    }

    /**
     * Get the size of the file
     * 
     * @param fileName The name of the file
     * @return The actual size of the file
     */
    size_t fileSize(const char *fileName)
    {
        if (SPIFFS.exists(fileName))
        {
            File file = openFile(fileName);
            size_t t = file.size();
            file.close();
            return t;
        }
        return 0;
    }

    /**
     * Open the file for reading or writing
     * 
     * @param fileName The name of the file
     * @param readOnly The file is readOnly or can be written to
     * @return The File Handle
     */
    File openFile(const char *filename, bool readOnly)
    {
        return SPIFFS.open(filename, readOnly ? "r" : "w");
    }

    /**
     * Compare two string, case or case insensitive
     * 
     * @param left The string to be compared against
     * @param right The string to be compared to
     * @param ignoreCase Ignoring case or not
     * @return True if the same.
     */
    bool compare(const char *left, const char *right, bool ignoreCase)
    {
        auto s1 = String(left);
        auto s2 = String(right);
        if( ignoreCase)
        {
            s1.toLowerCase();
            s2.toLowerCase();
        }

        return s1 == s2;
    }
} // namespace Utilities