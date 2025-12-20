#pragma once



struct ChoiceOption {
    int selectedChoice = 0;
    const char **optionStrings;
    int numOptions;

    void SelectNext() {if (++selectedChoice == numOptions) selectedChoice = 0; }
    const char* GetSelectedString();
};


struct IntOption {
    int value = 0;
    int min = 0;
    int max = 0;
    void Increase();
    void Decrease();
    void SetMin(int newMin);
    void SetMax(int newMax);
};
