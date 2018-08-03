import time
import math
import sys
import os
import platform

currdir = os.path.dirname(os.path.realpath(__file__))
logiciel_bord_path = currdir[0:currdir.index("LV")] + "LV/logiciel_bord/"
sys.path.append(logiciel_bord_path + "sources/protege/PRIMARY")

from options_locales import Options
from xml_perso import XMLPerso


class MenuConfig():


    def __init__(self):

        self.options = Options()
        self.path = self.options.path_read_only
        self.test_path = logiciel_bord_path + "test/"
        self.instruments_autorises = ["RACHUTS", "BECOOL", "BOLDAIR", "BBOP", "TSEN", "FLOATS",
                                        "LOAC", "ROC", "SAWFPHY", "PICOSDLA", "LPC"]
        self.baud_autorises = ["9600", "19200", "38400", "57600", "115200"]


    def start(self):

        print("################################################")
        print("########        CONFIGURATION        ###########")
        print("################################################")
        print("\n\n")


        new = input("New config ? [ y / n ]: ")
        new.lower()
        if new == "y":
            config = XMLPerso(nom_fichier=self.path + "superviseur.ini")
            self.instruments_autorises.sort()
            print("Authorized Instruments: \n")
            for element in self.instruments_autorises:
                print(element)
            print("\n\n")

            instrument = False

            while not instrument:
                inst_id = input("What is the name of the instrument ?: ")
                inst_id = inst_id.upper()
                if inst_id in self.instruments_autorises:
                    print("\nWelcome " + inst_id + "!\n")
                    instrument = True
                else:
                    print("\nunkown instrument\n")

            if self.options.MODE_SIMUL:
                print("What is the name of the serial port ?:\nexamples: \n")
                print("Window: COM1 or com1\nLinux: /dev/ttyS0\n")
                port = input("> ")
                if port.startswith("com"):
                    port = port.upper()
                print("serial port: " + port + "\n")
            else:
                port = "/dev/ttyUSB0"


            print("Which baud rate ?\n")
            for element in self.baud_autorises:
                print(element)
            baud = input("\n>")
            print("speed: " + baud + "\n")

            config.dico["SCI1"]["NOM"] = inst_id
            config.dico["SCI1"]["INTERFACE"] = port
            config.dico["SCI1"]["BAUD"] = baud
            config.ecrit_fichier(self.path + "superviseur.ini")

            print("configuration ready!")
            time.sleep(2)

        if platform.system() == "Windows":
            os.startfile(self.test_path + "start_LV.py")
        else:
            os.system("python " + self.test_path + "start_LV.py" + " &")
            
        

if __name__ == '__main__':
    SIMULATEUR = MenuConfig()
    SIMULATEUR.start()  
