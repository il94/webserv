#!/usr/bin/env python

import cgi
import os
import os.path

# Fonction pour générer le formulaire de connexion
def generate_login_form():
    print("Content-type: text/html")
    print("")
    with open('avocat_now/cgi_test/cgi-bin/login_content/login.html', 'r') as f:
        for ligne in f:
            print(ligne, end='')

# Fonction pour générer la page de déconnexion
def generate_logout_page():
    print("Content-type: text/html")
 
    print("Set-Cookie: session=expires")  # Supprimer le cookie de session
    print("Set-Cookie: password=expires")  # Supprimer le cookie de password
    print("")
    print("<html>")
    print("<body>")
    print("<h1>Déconnexion</h1>")
    print("<p>Vous êtes déconnecté.</p>")
    print("<p><a href=\"login.py\">Login</a></p>")
    print("</body>")
    print("</html>")

# Vérifier si le cookie de session existe

def check_session():
    if os.path.isfile('cookie.txt'):
        with open('cookie.txt', 'r') as f:
            output = f.read()
            if output.find("session=loggedin") != -1:
                return True
            return False
    return False


    # if 'HTTP_COOKIE' in os.environ:
    #     cookies = os.environ['HTTP_COOKIE'].split(';')
    #     for cookie in cookies:
    #         name, value = cookie.strip().split('=')
    #         if name == 'session' and value == 'loggedin':
    #             return True
    # return False



# def check_session():
#     if 'HTTP_COOKIE' in os.environ:
#         cookies = os.environ['HTTP_COOKIE'].split(';')
#         for cookie in cookies:
#             name, value = cookie.strip().split('=')
#             if name == 'session' and value == 'loggedin':
#                 return True
#     return False

# Vérifier les données de connexion
def check_credentials(username, password):
    # Vérification factice des données (exemple)
    if username != 'Zbub':
        return True
    return False

# Récupérer les données POST
form = cgi.FieldStorage()

# Vérifier si l'utilisateur est déjà connecté
if check_session() == True :
    # Afficher la page de déconnexion si l'utilisateur est déjà connecté
        
    # print('======================== COOKIE EXISTE ==================================')

    generate_logout_page()
else:
 
    # print('======================== COOKIE EXISTE PAS ==================================')
 
    # Vérifier si les données de connexion ont été soumises
    if 'username' in form and 'password' in form:

        # print('======================== DATA OBTENUE ==================================')

        username = form.getvalue("username")
        password = form.getvalue("password")

        # Vérifier les informations de connexion
        if check_credentials(username, password):
           
            # print('======================== CREATION DE COOKIE ==================================')
           
            # Créer un cookie de session
            print("Content-type: text/html")
            print("Set-Cookie: session=loggedin")  # Définir le cookie de session
            print("Set-Cookie: username=", username)  # Définir le cookie de user
            print("Set-Cookie: password=", password)  # Définir le cookie de password
            print("")

            # with open('avocat_now/cgi_test/cgi-bin/login_content/session_active.html', 'r') as f:
            #     for ligne in f:
            #         print(ligne, end='')



            print("<html>")
            print("<body>")
            print("<h1>Connexion réussie</h1>")
            print("<p>Bienvenue, {}!</p>".format(username))
            print("<p><a href=\"login.py\">Se déconnecter</a></p>")
            print("</body>")
            print("</html>")
        else:
            # Afficher à nouveau le formulaire de connexion avec un message d'erreur
            print("Content-type: text/html")
            print("")
            with open('avocat_now/cgi_test/cgi-bin/login_content/login_invalid.html', 'r') as f:
                for ligne2 in f:
                    print(ligne2, end='')
    else:
        # Afficher le formulaire de connexion
        # print('======================== DEMANDE DATA ==================================')
        generate_login_form()