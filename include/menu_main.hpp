#ifndef MENU_MAIN_HPP
#define MENU_MAIN_HPP

class MenuMain {
public:
    enum class Difficulty { CONTINUE, EASY, HARD, CHANGE_COMMANDS, EXIT };
    
    /*
     * Interactive arrow-key menu.
     */
    static Difficulty displayMenu();
    static void displayContribute();
    static void displayChangeCommands();
};

#endif // MENU_MAIN_HPP
