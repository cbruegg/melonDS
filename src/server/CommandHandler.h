//
// Created by mail on 09/05/2020.
//

#ifndef MELONDS_COMMANDHANDLER_H
#define MELONDS_COMMANDHANDLER_H


#include <string>
#include <utility>
#include <vector>

class CommandType {
public:
    static const CommandType Stop;
    static const CommandType Pause;
    static const CommandType Resume;
    static const CommandType KeyPress;
    static const CommandType KeyRelease;
    static const CommandType Touch;
    static const CommandType TouchRelease;
    static const CommandType ResetInput;
    static const CommandType SaveGameSave;
    static const CommandType LoadGameSave;
    static const CommandType SaveState;
    static const CommandType LoadState;
    static const CommandType AddCheat;
    static const CommandType ResetCheats;
    static const CommandType SetSpeed;
    static const CommandType Malformed;
    static const std::vector<const CommandType *> VALUES;

    const std::string id;
private:
    explicit CommandType(std::string id);
};

class Command {
public:
    const CommandType &commandType;
    void *commandData;

    Command(const CommandType &commandType, void *commandData);
};

class SetSpeedCommandData {
public:
    const double speed;

    explicit SetSpeedCommandData(double speed);
};

Command parseCommand(const std::string &line);

#endif //MELONDS_COMMANDHANDLER_H
