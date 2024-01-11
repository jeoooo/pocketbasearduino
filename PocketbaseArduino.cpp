#include "PocketbaseArduino.h"
#include <ESP8266HTTPClient.h>

// PocketbaseArduino Implementation

/**
 *
 * * *************************************************************************************************************
 * *                                               IMPORTANT:                                                    *
 * *                                                                                                             *
 * * You need at least two files for a library:                                                                  *
 * *                                                                                                             *
 * * 1. A header file (w/ the extension .h) and the source file (w/ extension .cpp).                             *
 * *    The header file has definitions for the library: basically a listing of everything that's inside;        *
 * *                                                                                                             *
 * * 2. source file - while the source file has the actual code.                                                 *
 * *                                                                                                             *
 * * source: https://docs.arduino.cc/learn/contributions/arduino-creating-library-guide                          *
 * *                                                                                                             *
 * * *************************************************************************************************************
 *
 */

/**
 * @brief Constructs a PocketbaseArduino object with the specified base URL.
 *
 * @param BASE_URL The base URL of the PocketBase API.
 */
PocketbaseArduino::PocketbaseArduino(const char *BASE_URL)
    : pb(BASE_URL) {}

/**
 * @brief Creates a PocketbaseCollection object for the specified collection name.
 *
 * @param collectionName The name of the collection.
 * @return PocketbaseCollection The PocketbaseCollection object.
 */
PocketbaseCollection PocketbaseArduino::collection(const char *collectionName)
{
    return PocketbaseCollection(pb, collectionName);
}

/**
 * @brief Fetchesa paginated records listing
 *
 * This function retrieves a paginated record from a Pocketbase collection
 * It allows optional parameters for page, perPage, sorting, filtering, expanding, querying specific fields, and skipTotal
 *
 * @param page      (Optional) The page (aka. offset) of the paginated list (default to 1).
 * @param perPage   (Optional) The max returned records per page (default to 30).
 * @param sort      (Optional) Specify the ORDER BY fields.
 * @param filter    (Optional) Filter expression to filter/search the returned records list (in addition to the collection's listRule)
 * @param expand    (Optional) Auto expand record relations.
 * @param fields    (Optional) Comma separated string of the fields to return in the JSON response (by default returns all fields).
 * @param skipTotal (Optional) If it is set the total counts query will be skipped and the response fields totalItems and totalPages will have -1 value.
 *                  This could drastically speed up the search queries when the total counters are not needed or cursor based pagination is used.
 *                  For optimization purposes, it is set by default for the getFirstListItem() and getFullList() SDKs methods.
 *
 * @return          A JSON String representing the paginated record from a Pocketbase collection
 */
const String PocketbaseCollection::getList(unsigned int page, unsigned int perPage, const char *sort, const char *filter, const char *expand, const char *fields, bool skipTotal)
{
    try
    {
        // Construct the API endpoint URL with optional parameters
        String apiUrl = pb.getBaseUrl() + "/api/collections/" + collectionName + "/records";

        apiUrl += "?page=" + String(page);
        apiUrl += "&perPage=" + String(perPage);

        // Add other optional parameters as needed
        if (sort != nullptr)
        {
            apiUrl += "&sort=" + String(sort);
        }

        if (filter != nullptr)
        {
            apiUrl += "&filter=" + String(filter);
        }

        if (expand != nullptr)
        {
            apiUrl += "&expand=" + String(expand);
        }

        if (fields != nullptr)
        {
            apiUrl += "&fields=" + String(fields);
        }

        if (skipTotal)
        {
            apiUrl += "&skipTotal=1";
        }

        // Check WiFi connection status and make the HTTP request
        if (WiFi.status() == WL_CONNECTED)
        {
            WiFiClient client;
            HTTPClient http;

            // Your Domain name with URL path or IP address with path
            http.begin(client, apiUrl.c_str());

            // Send HTTP GET request
            int httpResponseCode = http.GET();

            if (httpResponseCode > 0)
            {
                Serial.print("HTTP Response code: ");
                Serial.println(httpResponseCode);
                String result = http.getString();
                return result;
            }
            else
            {
                Serial.print("Error code: ");
                Serial.println(httpResponseCode);
            }

            // Free resources
            http.end();
        }
        else
        {
            Serial.println("WiFi Disconnected");
        }
    }
    catch (const std::exception &e)
    {
        Serial.println("Exception occurred during the request: " + String(e.what()));
    }

    // Return an empty string in case of an error
    return "";
}

/**
 * @brief Fetches a single record from a Pocketbase collection.
 *
 * This function retrieves a single record from a Pocketbase collection based on the provided record ID.
 * It allows optional parameters for expanding related fields and selecting specific fields to include in the response.
 *
 * @param recordId  The ID of the record to fetch.
 * @param expand    (Optional) A string specifying related fields to expand in the response.
 * @param fields    (Optional) A string specifying the fields to include in the response.
 *
 * @return          The fetched record as a String object.
 */
const String PocketbaseCollection::getOne(const char *recordId, const char *expand, const char *fields)
{
    try
    {
        // Construct the API endpoint URL with optional expand and fields query parameters
        String apiUrl = pb.getBaseUrl() + "/api/collections/" + collectionName + "/records/" + recordId;

        if (expand != nullptr)
        {
            apiUrl += "?expand=" + String(expand);
            if (fields != nullptr)
            {
                apiUrl += "&fields=" + String(fields);
            }
        }
        else if (fields != nullptr)
        {
            apiUrl += "?fields=" + String(fields);
        }

        // Perform the GET request to retrieve the record
        // Use your PocketBase library to make the GET request, and store the response in 'result'
        // For example, assuming pb has a getRecord method in your PocketBase library:
        String result = pb.getRecord(apiUrl);

        // Parse the result to check for error responses
        DynamicJsonDocument jsonResponse(1024); // Adjust the size based on your needs
        deserializeJson(jsonResponse, result);

        int code = jsonResponse["code"].as<int>();

        // Handle possible error responses
        if (code == 404)
        {
            // Handle 404 error
            throw PocketbaseArduinoException("Error: The requested resource wasn't found.");
        }
        else if (code == 403)
        {
            // Handle 403 error
            throw PocketbaseArduinoException("Error: Only admins can access this action.");
        }
        else
        {
            // Return the result
            return result;
        }
    }
    catch (const std::exception &e)
    {
        Serial.println("Exception occurred during the request: " + String(e.what()));
    }

    // Return an empty string in case of an error
    return "";
}

/**
 * @brief Creates a single, new record in the Pocketbase collection.
 *
 * @param jsonData  The JSON data for the new record.
 * @param id        (Optional) The ID of the record. If not set, it will be auto-generated by Pocketbase.
 * @param expand    (Optional) A string specifying related fields to expand in the response.
 * @param fields    (Optional) A string specifying the fields to include in the response.
 *
 * @return          True if the record was successfully created, false otherwise.
 */
const String PocketbaseCollection::create(const char *jsonData, const char *id, const char *expand, const char *fields)
{
    try
    {
        // Perform the actual record creation
        bool success = pb.addRecord(collectionName, jsonData, id, expand, fields);

        if (!success)
        {
            // Handle the case where the record creation was not successful
            throw PocketbaseArduinoException("Error: Failed to create record.");
        }

        // If successful, return an empty string (or customize as needed)
        return "";
    }
    catch (const PocketbaseArduinoException &e)
    {
        // Handle your custom exception
        Serial.println(e.what());
    }
    catch (const std::exception &e)
    {
        // Handle other standard exceptions
        Serial.println("Exception occurred during the request: " + String(e.what()));
    }

    return "";
}

// TODO: Implement update and deleteRecord functions for PocketbaseCollection
