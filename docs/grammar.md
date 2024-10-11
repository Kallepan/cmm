$$
\begin{align}
[\text{prog}] &\to [\text{stmt}]^* \\
[\text{stmt}] &\to \begin{cases}
    \text{exit}([\text{expr}]); \\
    \text{let ident} = [\text{expr}]; \\
\end{cases} \\
[\text{expr}] &\to 
\begin{cases}
\text{int\_literal} \\
\text{ident} \\
\end{cases}
\end{align}
$$