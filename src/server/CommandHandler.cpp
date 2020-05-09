//
// Created by mail on 09/05/2020.
//

#include <sstream>
#include <iostream>
#include "CommandHandler.h"


CommandType::CommandType(std::string id) : id(std::move(id)) {}

const CommandType CommandType::Stop = CommandType("Stop");
const CommandType CommandType::Pause = CommandType("Pause");
const CommandType CommandType::Resume = CommandType("Resume");
const CommandType CommandType::KeyPress = CommandType("KeyPress");
const CommandType CommandType::KeyRelease = CommandType("KeyRelease");
const CommandType CommandType::Touch = CommandType("Touch");
const CommandType CommandType::TouchRelease = CommandType("TouchRelease");
const CommandType CommandType::ResetInput = CommandType("ResetInput");
const CommandType CommandType::SaveGameSave = CommandType("SaveGameSave");
const CommandType CommandType::LoadGameSave = CommandType("LoadGameSave");
const CommandType CommandType::SaveState = CommandType("SaveState");
const CommandType CommandType::LoadState = CommandType("LoadState");
const CommandType CommandType::AddCheat = CommandType("AddCheat");
const CommandType CommandType::ResetCheats = CommandType("ResetCheats");
const CommandType CommandType::SetSpeed = CommandType("SetSpeed");
const CommandType CommandType::Malformed = CommandType("");
const std::vector<const CommandType *> CommandType::VALUES = {&CommandType::Stop, &CommandType::Pause,
                                                              &CommandType::Resume,
                                                              &CommandType::KeyPress, &CommandType::KeyRelease,
                                                              &CommandType::Touch, &CommandType::TouchRelease,
                                                              &CommandType::ResetInput, &CommandType::SaveGameSave,
                                                              &CommandType::LoadGameSave, &CommandType::SaveState,
                                                              &CommandType::LoadState,
                                                              &CommandType::AddCheat, &CommandType::ResetCheats,
                                                              &CommandType::SetSpeed, &CommandType::Malformed};

Command::Command(const CommandType &commandType, void *commandData) : commandType(commandType),
                                                                      commandData(commandData) {}

std::vector<std::string> split(const std::string &str, char delim) {
    std::stringstream test(str);
    std::string part;
    std::vector<std::string> parts;

    while (std::getline(test, part, delim)) {
        parts.push_back(part);
    }

    return parts;
}

Command parseCommand(const std::string &line) {
    std::vector<std::string> cmd = split(line, ' ');
    const auto commandTypeStr = cmd.at(0);

    const CommandType *commandType = &CommandType::Malformed;
    for (const auto commandTypeCandidate : CommandType::VALUES) {
        if (commandTypeStr.find(commandTypeCandidate->id) != std::string::npos) {
            std::cout << "Found " << commandTypeCandidate->id << std::endl;
            commandType = commandTypeCandidate;
            break;
        }
    }

    if (commandType == &CommandType::Stop) {
        return Command(*commandType, nullptr);
    } else if (commandType == &CommandType::Pause) {
        return Command(*commandType, nullptr);
    } else if (commandType == &CommandType::Resume) {
        return Command(*commandType, nullptr);
    } else if (commandType == &CommandType::KeyPress) {
        return Command(*commandType, nullptr);
    } else if (commandType == &CommandType::KeyRelease) {
        return Command(*commandType, nullptr);
    } else if (commandType == &CommandType::Touch) {
        return Command(*commandType, nullptr);
    } else if (commandType == &CommandType::TouchRelease) {
        return Command(*commandType, nullptr);
    } else if (commandType == &CommandType::ResetInput) {
        return Command(*commandType, nullptr);
    } else if (commandType == &CommandType::SaveGameSave) {
        return Command(*commandType, nullptr);
    } else if (commandType == &CommandType::LoadGameSave) {
        return Command(*commandType, nullptr);
    } else if (commandType == &CommandType::SaveState) {
        return Command(*commandType, nullptr);
    } else if (commandType == &CommandType::LoadState) {
        return Command(*commandType, nullptr);
    } else if (commandType == &CommandType::AddCheat) {
        return Command(*commandType, nullptr);
    } else if (commandType == &CommandType::ResetCheats) {
        return Command(*commandType, nullptr);
    } else if (commandType == &CommandType::SetSpeed) {
        std::cout << "DEBUG: Parsed SetSpeed" << std::endl;
        auto commandData = new SetSpeedCommandData(std::stod(cmd.at(1)));
        return Command(*commandType, commandData);
    } else if (commandType == &CommandType::Malformed) {
        return Command(*commandType, nullptr);
    }

    return Command(CommandType::Malformed, nullptr);
}


SetSpeedCommandData::SetSpeedCommandData(const double speed) : speed(speed) {}
