$$
\begin{align}
[\text{prog}] &\to [\text{stmt}]^* \\
[\text{stmt}] &\to \begin{cases}
    \text{exit}([\text{expr}]); \\
    \text{let ident} = [\text{expr}]; \\
\end{cases} \\
[\text{expr}] &\to \text{int\_literal}
\end{align}
$$