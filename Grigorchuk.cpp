// MyIndiva.cpp: определяет точку входа для консольного приложения.
//
#define _CRT_SECURE_NO_DEPRECATE 
#include "stdafx.h"
#include <iostream>
#include <omp.h>
#include <stdio.h>
#include <fstream>
#include <string>
#include "omp.h"


using namespace std;
const int SizeM = 300; //Размер матрицы 
const bool PrintInfo = false;

void createLowTriangleM(FILE *filename,int size)
{
	for (int i = 0; i < size; ++i) // цикл по строкам a
	{
		for (int j = 0; j < size; ++j) // цикл по столбцам
		{
			if (j <= i)
				fprintf(filename, "%5i", 1 + rand() % 15);
			else
				fprintf(filename, "%5i", 0);
		}
		fprintf(filename, "\n");
	}
}

//чтение из файла в массив
void massFromFile(int* &mass, const int size, FILE *filename)
{
	while (!feof(filename))
		for (int i = 0; i<size; ++i) 
			for (int j = 0; j<size; ++j) 
				fscanf(filename, "%5d", &mass[i*size + j]);
}


void print_arr(const int a[], int n) //Вывод массива на экран
{
	for (int i = 0; i < n; ++i)
	{
		for (int j = 0; j < n; ++j) {
			cout.width(5);
			cout << a[i*n + j];
		}
		cout << endl;
	}
}

void print_vec(const int a[], int n)
{
	for (int i = 0; i < n; ++i)
	{
		cout.width(5);
		cout << a[i];
	}
	cout << endl;
}

//Из считанной строки А делаем строку по блочным столбцам
void MassToBlock(int block, int BlockMass[], const int mass[], int size)
{
	int first=0, second=0;
	for (int j = 0; j<size / block; ++j)
		for (int i = j * block; i<size; ++i)
			for (int s = 0; s<block; ++ s)
			{
				first = s + i * size + j * block ;
				BlockMass[second] = mass[first];
				++second;
			}
}

//Перемножение двух блоков
void blockMultiply(int *res, const int *first, const int *second, int blockSize)
{
	int  tmp;
	for (int i = 0; i < blockSize; ++i)
		for (int j = 0; j < blockSize; ++j)
		{
			tmp = 0;
			for (int k = 0; k < blockSize; ++k)
				tmp += first[i*blockSize + k] * second[k*blockSize + j];
			res[i*blockSize + j] = tmp;
		}
}

void printMatrix(const int matrix[], int blockSize, int size) 
{
	int blockCount = size / blockSize;
	for (int i = 0; i < blockCount; ++i)
		for (int s=0; s < blockSize; ++s)
		{
			for (int j = 0; j < blockCount; ++j)
				for (int jb(0); jb < blockSize; ++jb)
				{
					cout.width(5);
					cout << matrix[i*blockSize*size + j * blockSize*blockSize + s * blockSize + jb];
				}
			cout << endl;
		}
}

//Стандартное умножение блочной матрицы
int* StandartBlockMultiplying(const int *first, const int *second, const int size)
{
	int *result = new int[SizeM*SizeM];

	for (int i= 0; i < SizeM*SizeM; ++i)
		result[i] = 0;

	int m = SizeM / size; 
	int size_a = ((m*m - m) / 2 + m)*size*size; 
	//из массива в блок
	int *firstBlocked = new int[size_a], *secondBlocked = new int[size_a];
	MassToBlock(size, firstBlocked, first, SizeM);
	MassToBlock(size, secondBlocked, second, SizeM);

	int *blockFirst = new int[size*size], *blockSecond = new int[size*size];
	int *blockResult = new int[size*size];
	int k;
	double timeBegin = omp_get_wtime(), timeEnd;

	for (int i = 0; i < m; ++i)
		for (int j = 0; j <= i; ++j)
		{
			int *blockTmp = result + i * m*size*size + j * size*size;
			//зануление блока
			for (int s = 0; s < size*size; s++)
				blockTmp[s] = 0;

			for (k = j; k <= i; ++k)
			{
				//вычисление начало необходимого блока
				int firstBlPosition = (2 * m*k - k * k - k) / 2;
				int SecondBlPosition = j + (2 * m*j - j * j - 3 * j) / 2 + k;
				for (int s = 0; s < size*size; ++s)
				{
					blockFirst[s] = firstBlocked[(i + firstBlPosition)*size*size + s];
					blockSecond[s] = secondBlocked[(SecondBlPosition)*size*size + s];
				}
				blockMultiply(blockResult, blockFirst, blockSecond, size);
				for (int s = 0; s < size*size; ++s)
					blockTmp[s] = blockTmp[s] + blockResult[s];
			}


		}
	delete[] blockFirst;
	delete[] blockSecond;
	delete[] blockResult;

	timeEnd = omp_get_wtime();
	if (PrintInfo)
	{
		cout << "\n------------------------ Size= " << size << "\n";
		printMatrix(result, size, SizeM);
		cout << "\n-------------------------------------------\n";
	}
	
	FILE* file = fopen("timeBlock.txt", "a");
	fprintf(file, "%f\n", (timeEnd - timeBegin));
	fclose(file);
	return result;
}

int* ParallelBlockMultiplying(const int *first, const int *second, const int size)
{
	int *result = new int[SizeM*SizeM];

	for (int i = 0; i < SizeM*SizeM; ++i)
		result[i] = 0;

	int m = SizeM / size;
	int size_a = ((m*m - m) / 2 + m)*size*size;
	//из массива в блок
	int *firstBlocked = new int[size_a], *secondBlocked = new int[size_a];
	MassToBlock(size, firstBlocked, first, SizeM);
	MassToBlock(size, secondBlocked, second, SizeM);

	int k;
	double timeBegin = omp_get_wtime(), timeEnd;

	for (int i = 0; i < m; ++i)
		for (int j = 0; j <= i; ++j)
		{
			int *blockTmp = result + i * m*size*size + j * size*size;
			//зануление блока
			for (int s = 0; s < size*size; s++)
				blockTmp[s] = 0;
#pragma omp parallel num_threads(8) 
			{
#pragma omp for  schedule(static)
				for (k = j; k <= i; ++k)
				{
					//вычисление начало необходимого блока
					int firstBlPosition = (2 * m*k - k * k - k) / 2;
					int SecondBlPosition = j + (2 * m*j - j * j - 3 * j) / 2 + k;
					int *blockFirst = firstBlocked + (i + firstBlPosition)*size*size ;
					int *blockSecond = secondBlocked + (SecondBlPosition)*size*size;
					int *blockResult = new int[size*size];
					if (k < i)
						blockMultiply(blockResult, blockFirst, blockSecond, size);
					for (int s = 0; s < size*size; ++s)
						blockTmp[s] = blockTmp[s] + blockResult[s];
					delete[] blockResult;
				}
			}

		}

	timeEnd = omp_get_wtime();
	if (PrintInfo)
	{
		cout << "\n------------------------ Size= " << size << "\n";
		printMatrix(result, size, SizeM);
		cout << "\n-------------------------------------------\n";
	}

	FILE* file = fopen("timeParallelBlock.txt", "a");
	fprintf(file, "%f\n", (timeEnd - timeBegin));
	fclose(file);
	return result;
}

//Функция блочного перемножения строк двух матриц
int* ParallelInBlockMultiplying(const int *first, const int *second, const int size)
{
	int *result = new int[SizeM*SizeM];

	for (int i = 0; i < SizeM*SizeM; ++i)
		result[i] = 0;

	int m = SizeM / size;
	int size_a = ((m*m - m) / 2 + m)*size*size;
	//из массива в блок
	int *firstBlocked = new int[size_a], *secondBlocked = new int[size_a];
	MassToBlock(size, firstBlocked, first, SizeM);
	MassToBlock(size, secondBlocked, second, SizeM);

	int k;
	double timeBegin = omp_get_wtime(), timeEnd;

#pragma omp parallel num_threads(8) 
	{
#pragma omp for  schedule(static)
		for (int i = 0; i < m; ++i)
			for (int j = 0; j <= i; ++j)
			{
				int *blockTmp = result + i * m*size*size + j * size*size;
				//зануление блока
				for (int s = 0; s < size*size; s++)
					blockTmp[s] = 0;

				for (k = j; k <= i; ++k)
				{
					//вычисление начало необходимого блока
					int firstBlPosition = (2 * m*k - k * k - k) / 2;
					int SecondBlPosition = j + (2 * m*j - j * j - 3 * j) / 2 + k;
					int *blockFirst = firstBlocked + (i + firstBlPosition)*size*size;
					int *blockSecond = secondBlocked + (SecondBlPosition)*size*size;
					int *blockResult = new int[size*size];
					if (k < i)
						blockMultiply(blockResult, blockFirst, blockSecond, size);
					for (int s = 0; s < size*size; ++s)
						blockTmp[s] = blockTmp[s] + blockResult[s];
					delete[] blockResult;
				}


			}
	}

	timeEnd = omp_get_wtime();
	if (PrintInfo)
	{
		cout << "\n------------------------ Size= " << size << "\n";
		printMatrix(result, size, SizeM);
		cout << "\n-------------------------------------------\n";
	}

	FILE* file = fopen("timeParallelInBlock.txt", "a");
	fprintf(file, "%f\n", (timeEnd - timeBegin));
	fclose(file);
	return result;
}



void StandartMultiplying(int *first, int *second)
{
	int *result = new int[SizeM*SizeM];
	double timeBegin, timeEnd;
	timeBegin = omp_get_wtime();
	blockMultiply(result, first, second, SizeM);
	timeEnd = omp_get_wtime();
	FILE* file = fopen("timeStandartNOBlock.txt", "a");
	fprintf(file, "%f\n", (timeEnd - timeBegin));
	fclose(file);
	//print_arr(result, SizeM);
	delete[] result;
}


int main()
{
	setlocale(LC_ALL, "rus");
	FILE *fileMA = fopen("AMatrix.txt", "w");
	createLowTriangleM(fileMA, SizeM);
	fclose(fileMA);

	FILE *fileMB = fopen("BMatrix.txt", "w");
	createLowTriangleM(fileMB, SizeM);
	fclose(fileMB);
	
	fileMA = fopen("AMatrix.txt", "r");
	int *aMass = new int[SizeM*SizeM];
	for (int i = 0; i < SizeM*SizeM; i++)
		aMass[i] = 0;
	massFromFile(aMass, SizeM, fileMA);
	fclose(fileMA);

	fileMB = fopen("BMatrix.txt", "r");
	int *bMass = new int[SizeM*SizeM];
	for (int i = 0; i < SizeM*SizeM; i++)
		bMass[i] = 0;
	massFromFile(bMass, SizeM, fileMB);
	fclose(fileMB);

	
	StandartMultiplying(aMass, bMass);
	
	for (int dimensionBlock = 1; dimensionBlock <= SizeM; ++dimensionBlock)
		if (SizeM%dimensionBlock == 0)
		{
			int *result = StandartBlockMultiplying(aMass, bMass, dimensionBlock);
			delete[] result;

			FILE* file = fopen("blockcount.txt", "a");
			fprintf(file, "%5d\n", (dimensionBlock));
			fclose(file);
		}
	for (int dimensionBlock = 1; dimensionBlock <= SizeM; ++dimensionBlock)
		if (SizeM%dimensionBlock == 0)
		{
			int *resultPBL = ParallelBlockMultiplying(aMass, bMass, dimensionBlock);
			delete[] resultPBL;
		}

	for (int dimensionBlock = 1; dimensionBlock <= SizeM; ++dimensionBlock)
		if (SizeM%dimensionBlock == 0)
		{
			int *resultPINBL = ParallelInBlockMultiplying(aMass, bMass, dimensionBlock);
			delete[] resultPINBL;
		}
	
	delete[] aMass;
	delete[] bMass;
	system("pause");
	return 0;
}

