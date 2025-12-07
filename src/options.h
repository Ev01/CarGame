#pragma once



struct ChoiceOption {
    int selectedChoice = 0;
    const char **optionStrings;
    int numOptions;

    void SelectNext() {if (++selectedChoice == numOptions) selectedChoice = 0; }
};
