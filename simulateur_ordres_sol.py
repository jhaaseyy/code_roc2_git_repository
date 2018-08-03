import time
import math
import sys
import os
#import ctypes

#ctypes.windll.kernel32.SetConsoleTitleW("ORDER SIMULATOR")

currdir = os.path.dirname(os.path.realpath(__file__))
logiciel_bord_path = currdir[0:currdir.index("logiciel_bord")] + "logiciel_bord/"
sys.path.append(logiciel_bord_path + "sources/protege/PRIMARY")

from options_locales import Options
from xml_perso import XMLPerso



class SimulateurCCMTC():


    def __init__(self):

        self.options = Options()
        self.path = self.options.dico_path["TC"]
        self.temp_path = self.options.dico_path_temp["TC"]
        self.read_only = self.options.path_read_only

        self.dico_ccm_tc_autorises = {
            "GO_EOF": self.envoie_ccm_tc_exp,
            "GO_SAFE": self.envoie_ccm_tc_exp,
            "SEND_SW": self.envoie_ccm_tc_exp,
            "GO_LOW_POWER": self.envoie_ccm_tc_exp,
            "GO_FLIGHT": self.envoie_ccm_tc_exp,
            "GO_STANDBY": self.envoie_ccm_tc_exp,
            "SEND_FILE": self.envoie_ccm_tc_exp
        }

        self.dico_zephyr_ordres_autorises = {
            "BAT_LOW": self.envoie_ccm_tc,
            "BAT_OK": self.envoie_ccm_tc,
            "SHOW_LOG": self.show_log
        }


        self.instruments_autorises = ["RACHUTS", "BECOOL", "BOLDAIR", "BBOP", "TSEN", "FLOATS",
                                        "LOAC", "ROC", "SAWFPHY", "PICOSDLA", "LPC"] 

    def envoie_ccm_tc_exp(self, inst_id, ordre):

        ordre_exp = "OBCZ.EXP[" + inst_id + "]." + ordre
        self.envoie_ccm_tc(ordre_exp)
            

    def envoie_ccm_tc(self, ordre):

        nom_fichier = "TC." + time.strftime("%y%m%d%H%M%S") + ".data"

        with open(self.temp_path + nom_fichier + ".WAIT", "w"):
            pass

        with open(self.path + nom_fichier, "w") as file:
            file.write("<CCM_TC>" + ordre + "</CCM_TC>\n")
            
        with open(self.temp_path + nom_fichier + ".NEW", "w"):
            pass

        os.remove(self.temp_path + nom_fichier + ".WAIT")

    def show_log(self, ordre):
        log_list = []
        filelist = os.listdir(logiciel_bord_path + "exec/pathsuperviseur")
        for fichier in filelist:
            if fichier.endswith("log"):
                log_list.append(fichier)
        log_list.sort()
        for log in log_list:
            os.system("cat " + logiciel_bord_path + "exec/pathsuperviseur/" + log)





    def start(self):

        print("######################################################################")
        print("########        SOL >>))>>))>>))>>))>>))>>))>>)) BORD     ############")
        print("######################################################################")
        print("\n\n")

        config = XMLPerso(nom_fichier=self.read_only + "superviseur.ini")
        inst_id = config.dico["SCI1"]["NOM"]
        print("\nWelcome " + inst_id + "!\n")


        print("Authorized Orders: \n")
        for element in self.dico_ccm_tc_autorises.keys():
            print(element)

        print("\n\n")

        print("Authorized Actions: \n")
        for element in self.dico_zephyr_ordres_autorises.keys():
            if element.startswith("SHOW"):
                print(element)
            else:
                print(element + " (RACHUTS only)")

        print("\n\n")

        while 1:            
            time.sleep(1)
            ordre = input("which order / action do you want to send ?: ")
            if ordre in self.dico_ccm_tc_autorises.keys():
                self.dico_ccm_tc_autorises[ordre](inst_id, ordre)
                print("sent!\n")
            elif ordre in self.dico_zephyr_ordres_autorises.keys():
                self.dico_zephyr_ordres_autorises[ordre](ordre)
                print("sent!")
            else:
                print("unknown order\n")


if __name__ == '__main__':
    SIMULATEUR = SimulateurCCMTC()
    SIMULATEUR.start()   
