// AddressSanitizer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

int main()
{
    int *array = new int[100];
    array[100] = 1; // 0-99�ȊO�̃A�N�Z�X�Ȃ̂ŁA�͈͊O�A�N�Z�X
}
