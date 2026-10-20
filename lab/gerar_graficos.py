import pandas as pd
import matplotlib.pyplot as plt

# 1. Configurações base
PERIODO_NOMINAL_MS = 50.0  # Período da amostragem de y
COLUNAS = ['Tempo_us', 'y1', 'y2', 'y3']

def analisar_dados(arquivo, nome_cenario):
    # Lê o arquivo ignorando erros de formatação
    df = pd.read_csv(arquivo, sep='\t', header=None, names=COLUNAS, engine='python').dropna()
    
    # Conversões de tempo
    df['Tempo_ms'] = df['Tempo_us'] / 1000.0
    df['Tempo_s'] = df['Tempo_us'] / 1e6
    
    # Cálculos de Período T(k) e Jitter J(k)
    df['T_k'] = df['Tempo_ms'].diff()
    df['J_k'] = df['T_k'] - PERIODO_NOMINAL_MS
    df = df.dropna() # Remove a primeira linha vazia gerada pelo diff()
    
    # Calcula as métricas estatísticas para a tabela
    stats = {
        'Cenário': nome_cenario,
        'T(k) Média': df['T_k'].mean(),
        'T(k) Var': df['T_k'].var(),
        'T(k) DesvPad': df['T_k'].std(),
        'T(k) Máx': df['T_k'].max(),
        'T(k) Mín': df['T_k'].min(),
        'J(k) Média': df['J_k'].mean(),
        'J(k) Var': df['J_k'].var(),
        'J(k) DesvPad': df['J_k'].std(),
        'J(k) Máx': df['J_k'].max(),
        'J(k) Mín': df['J_k'].min()
    }
    return df, stats

# 2. Processamento dos seus dois arquivos
df_sem, stats_sem = analisar_dados('test.dat', 'Sem Carga')
df_com, stats_com = analisar_dados('test_carga.dat', 'Com Carga')

# ==========================================
# ITEM 6: Geração da Tabela no Terminal
# ==========================================
tabela = pd.DataFrame([stats_sem, stats_com])
tabela.set_index('Cenário', inplace=True)

print("\n" + "="*90)
print("TABELA ESTATÍSTICA: PERÍODO T(k) E JITTER J(k) EM MILISSEGUNDOS")
print("="*90)
print(tabela.round(4).to_string())
print("="*90 + "\n")

# ==========================================
# ITENS 4 E 5: Geração dos Gráficos Individuais
# ==========================================

# Gráfico 1: Sinal y(t) amostrado
plt.figure(figsize=(8, 4))
plt.plot(df_sem['Tempo_s'], df_sem['y1'], label='y1 (Sem Carga)', color='blue')
plt.plot(df_com['Tempo_s'], df_com['y1'], label='y1 (Com Carga)', color='red', linestyle='dashed', alpha=0.8)
plt.xlabel('Tempo (s)')
plt.ylabel('Amplitude')
plt.title('Sinal Amostrado no Tempo')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig('sinal_amostrado.png', dpi=300)
plt.close()

# Gráfico 2: Período T(k)
plt.figure(figsize=(8, 4))
plt.plot(df_sem['Tempo_s'], df_sem['T_k'], label='Sem Carga', color='blue', marker='.', linestyle='none', alpha=0.5)
plt.plot(df_com['Tempo_s'], df_com['T_k'], label='Com Carga', color='red', marker='x', linestyle='none', alpha=0.5)
plt.axhline(y=PERIODO_NOMINAL_MS, color='black', linestyle='-', label='Período Nominal (50ms)')
plt.xlabel('Tempo (s)')
plt.ylabel('T(k) [ms]')
plt.title('Período Real de Execução')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig('periodo_tk.png', dpi=300)
plt.close()

# Gráfico 3: Jitter J(k)
plt.figure(figsize=(8, 4))
plt.plot(df_sem['Tempo_s'], df_sem['J_k'], label='Sem Carga', color='blue', marker='.', linestyle='none', alpha=0.5)
plt.plot(df_com['Tempo_s'], df_com['J_k'], label='Com Carga', color='red', marker='x', linestyle='none', alpha=0.5)
plt.axhline(y=0, color='black', linestyle='-')
plt.xlabel('Tempo (s)')
plt.ylabel('Jitter [ms]')
plt.title('Jitter da Amostragem')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig('jitter_jk.png', dpi=300)
plt.close()

print("Os 3 gráficos foram guardados como ficheiros PNG na sua pasta!")