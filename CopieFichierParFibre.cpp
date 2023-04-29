// CopieFichierParFibre.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//

#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <shlwapi.h>

#pragma comment (lib,"Shlwapi")

constexpr auto RETOUR_VALIDE = 0;
constexpr auto RETOUR_FONCTION = 0x01;
constexpr auto RETOUR_ERREUR = 0x0D;
constexpr auto TAMPON = 0x7FFF;
constexpr auto NOMBRE_FIBRE = 0x03;
constexpr auto FIBRE_PRIMAIRE = 0 ;
constexpr auto LIRE_FIBRE = 0x01 ;
constexpr auto ECRIRE_FIBRE = 0x02 ;

typedef struct
{
    DWORD wParametre;
    DWORD wResultat;
    HANDLE wFichier;
    DWORD wOctets;
} FIBERDATASTRUCT, * PFIBERDATASTRUCT, * LPFIBERDATASTRUCT;


VOID __stdcall LireFibre(LPVOID Parametre);
VOID __stdcall EcrireFibre(LPVOID Parametre);
VOID AfficherInfoFibre(VOID);

LPVOID m_lpFibre[NOMBRE_FIBRE];
LPBYTE m_Tampon;
DWORD m_OctetsLus;

int __cdecl _tmain(int argc, TCHAR* argv[])
{
    SetConsoleTitle("Copie de fichier par fibres");
    LPFIBERDATASTRUCT fs;
    printf("Copie de fichier par fibres\t v.1.0.0.1\n%cPatrice Waechter-Ebling 2023\n", 0xB8);
    if (argc != 3)
    {
        printf("Usage: %s <Fichier Source> <Fichier Destination>\n", PathFindFileName(argv[0]));
        return RETOUR_FONCTION;
    }
    fs = (LPFIBERDATASTRUCT)HeapAlloc(GetProcessHeap(), 0,sizeof(FIBERDATASTRUCT) * NOMBRE_FIBRE);
    if (fs == NULL)
    {
        printf("Erreur d'allocation (% d)\n", GetLastError());
        return RETOUR_ERREUR;
    }
    m_Tampon = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, TAMPON);
    if (m_Tampon == NULL)
    {
        printf("Erreur d'allocation pour le tampon (% d)\n", GetLastError());
        return RETOUR_ERREUR;
    }
    fs[LIRE_FIBRE].wFichier = CreateFile(argv[1],GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if (fs[LIRE_FIBRE].wFichier == INVALID_HANDLE_VALUE)
    {
        printf("Erreur d'acces au fichier Source(% d)\n", GetLastError());
        return RETOUR_ERREUR;
    }
    fs[ECRIRE_FIBRE].wFichier = CreateFile(argv[2],GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if (fs[ECRIRE_FIBRE].wFichier == INVALID_HANDLE_VALUE)
    {
        printf("Erreur d'acces au fichier de destination (% d)\n", GetLastError());
        return RETOUR_ERREUR;
    }
    m_lpFibre[FIBRE_PRIMAIRE] = ConvertThreadToFiber(&fs[FIBRE_PRIMAIRE]);
    if (m_lpFibre[FIBRE_PRIMAIRE] == NULL)
    {
        printf("Erreur de convertion Thread en Fibre (%d)\n", GetLastError());
        return RETOUR_ERREUR;
    }
    fs[FIBRE_PRIMAIRE].wParametre = 0;
    fs[FIBRE_PRIMAIRE].wResultat = 0;
    fs[FIBRE_PRIMAIRE].wFichier = INVALID_HANDLE_VALUE;
    m_lpFibre[LIRE_FIBRE] = CreateFiber(0, LireFibre, &fs[LIRE_FIBRE]);
    if (m_lpFibre[LIRE_FIBRE] == NULL)
    {
        printf("Erreur lors de la creation de la fibre de lecture (% d)\n", GetLastError());
        return RETOUR_ERREUR;
    }
    fs[LIRE_FIBRE].wParametre = 0x12345678;
    m_lpFibre[ECRIRE_FIBRE] = CreateFiber(0, EcrireFibre, &fs[ECRIRE_FIBRE]);
    if (m_lpFibre[ECRIRE_FIBRE] == NULL)
    {
        printf("Erreur lors de la creation de la fibre d'e lec' ecriture (% d)\n", GetLastError());
        return RETOUR_ERREUR;
    }
    fs[ECRIRE_FIBRE].wParametre = 0x54545454;
    SwitchToFiber(m_lpFibre[LIRE_FIBRE]);
    printf("LireFibre: Resultat %lu, %lu bits lus\n",fs[LIRE_FIBRE].wResultat, fs[LIRE_FIBRE].wOctets);
    printf("EcrireFibre: Resultat %lu, %lu bits ecris\n",fs[ECRIRE_FIBRE].wResultat, fs[ECRIRE_FIBRE].wOctets);
    DeleteFiber(m_lpFibre[LIRE_FIBRE]);
    DeleteFiber(m_lpFibre[ECRIRE_FIBRE]);
    CloseHandle(fs[LIRE_FIBRE].wFichier);
    CloseHandle(fs[ECRIRE_FIBRE].wFichier);
    return RETOUR_VALIDE;
}

VOID __stdcall LireFibre(LPVOID Parametre)
{
    LPFIBERDATASTRUCT fds = (LPFIBERDATASTRUCT)Parametre;
    if (fds == NULL)
    {
        printf("Un parametre null a ete passe Abandon.\n");
        return;
    }
    AfficherInfoFibre();
    fds->wOctets = 0;
    while (1)
    {
        if (!ReadFile(fds->wFichier, m_Tampon, TAMPON,&m_OctetsLus, NULL)){break;}
        if (m_OctetsLus == 0) break;
        fds->wOctets += m_OctetsLus;
        SwitchToFiber(m_lpFibre[ECRIRE_FIBRE]);
    } 
    fds->wResultat = GetLastError();
    SwitchToFiber(m_lpFibre[FIBRE_PRIMAIRE]);
}

VOID __stdcall EcrireFibre(LPVOID Parametre)
{
    LPFIBERDATASTRUCT fds = (LPFIBERDATASTRUCT)Parametre;
    DWORD dwBitsEcris;
    if (fds == NULL)
    {
        printf("Un parametre null a ete passe Abandon.\n");
        return;
    }
    AfficherInfoFibre();
    fds->wOctets = 0;
    fds->wResultat = ERROR_SUCCESS;
    while (1)
    {
        if (!WriteFile(fds->wFichier, m_Tampon, m_OctetsLus,&dwBitsEcris, NULL)){break;}
        fds->wOctets += dwBitsEcris;
        printf("\rEcriture de %lu sur %lu", fds->wOctets, dwBitsEcris);
        SwitchToFiber(m_lpFibre[LIRE_FIBRE]);
    } 
    fds->wResultat = GetLastError();
    SwitchToFiber(m_lpFibre[FIBRE_PRIMAIRE]);
}

VOID AfficherInfoFibre(VOID)
{
    LPFIBERDATASTRUCT fds = (LPFIBERDATASTRUCT)GetFiberData();
    LPVOID lpCurrentFiber = GetCurrentFiber();
    if (lpCurrentFiber == m_lpFibre[LIRE_FIBRE])
        printf("Lecture Fibre entree");
    else
    {
        if (lpCurrentFiber == m_lpFibre[ECRIRE_FIBRE])
            printf("Ecriture de la fibre entree");
        else
        {
            if (lpCurrentFiber == m_lpFibre[FIBRE_PRIMAIRE])
                printf("Fibre Primaire saisie");
            else
                printf("Fibre Inconnue");
        }
    }
    printf(" (wParametre est de  0x%lx)\n", fds->wParametre);
}