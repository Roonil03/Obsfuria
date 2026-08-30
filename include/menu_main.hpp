#ifndef MENU_MAIN_HPP
#define MENU_MAIN_HPP

class MenuMain {
public:
    enum class Difficulty { CONTINUE, EASY, HARD, CHANGE_COMMANDS, ADD_COMMANDS, EXIT };
    
    /*
     * Interactive arrow-key menu.
     */
    static Difficulty displayMenu();
    static void displayContribute();
    static void displayChangeCommands();
    static void displayAddCommands();
};

#endif // MENU_MAIN_HPP
