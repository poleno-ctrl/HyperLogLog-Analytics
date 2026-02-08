import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('hll_results.csv')

plt.style.use('ggplot') 
plt.figure(figsize=(18, 8))

plt.subplot(1, 2, 1)
plt.plot(df['Step'], df['Exact_Ref'], 'k--', label='True Count ($F_0^t$)', linewidth=2)
plt.plot(df['Step'], df['Avg_Est'], 'b-', label='HLL Estimate ($N_t$)', alpha=0.7)

plt.title('График №1: Точность HyperLogLog')
plt.xlabel('Шаг (Количество обработанных элементов)')
plt.ylabel('Количество уникальных элементов')
plt.legend()
plt.grid(True)

plt.subplot(1, 2, 2)

plt.plot(df['Step'], df['Avg_Est'], 'r-', label='Mean Estimate $\mathbb{E}(N_t)$')

plt.fill_between(
    df['Step'], 
    df['Lower_Bound'], 
    df['Upper_Bound'], 
    color='red', 
    alpha=0.2, 
    label='Uncertainty $\mathbb{E}(N_t) \pm \sigma_{N_t}$'
)

plt.plot(df['Step'], df['Exact_Ref'], 'k:', label='True Reference', alpha=0.5)

plt.title('График №2: Стабильность оценки (Дисперсия)')
plt.xlabel('Шаг')
plt.ylabel('Оценка количества уникальных')
plt.legend()
plt.grid(True)

plt.tight_layout()
plt.show()
