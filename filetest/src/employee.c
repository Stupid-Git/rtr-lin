/*
 * employee.c
 *
 *  Created on: Nov 6, 2024
 *      Author: karel
 */
// C program to read a struct from a binary file
#include <stdio.h>
#include <stdint.h>
#include <string.h>


typedef struct {
    int id;
    char name[50];
    float salary;
} Employee_t;

int employee_make_if_not_exist();
int employee_make_if_not_exist()
{
	FILE* file;
	Employee_t Employee;
    // open the file in rb mode
    file = fopen("employee_data.bin", "rb");

    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        //return 1;

        file = fopen("employee_data.bin", "wb");
        if (file == NULL) {
            perror("Error creating file");
            return 1;
        }

        Employee.id = 42;
        strcpy(Employee.name, "FRED");
        Employee.salary = 123.45;

        fwrite((char*)&Employee, sizeof(Employee_t), 1, file );

        Employee.id = 43;
        strcpy(Employee.name, "JOE");
        Employee.salary = 234.56;

        fwrite((char*)&Employee, sizeof(Employee_t), 1, file );

        fclose( file);
    }

    return 0;
}

int employee_read();
int employee_read()
{
    // open the file in rb mode
    FILE* file = fopen("employee_data.bin", "rb");

    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Define the struct
    Employee_t employee;

    // Read the structs present in the file
    while (fread(&employee, sizeof(Employee_t), 1, file)
           == 1) {
        // Process the read data (e.g., print or manipulate)
        printf("Employee ID: %d, Name: %s, Salary: %.2f\n",
               employee.id, employee.name, employee.salary);
    }
    // close the file
    fclose(file);
    return 0;
}

int employee_main();
int employee_main()
{

	 employee_make_if_not_exist();
	 employee_read();

	 return 0;
}
