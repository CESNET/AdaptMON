export default {
    extends: ['@commitlint/config-conventional'],
    rules: {
        "fix-requires-reference": [2, "always"],
    },
    plugins: [
        {
            rules: {
                'fix-requires-reference': (parsed) => {
                    const { type, body, footer } = parsed;

                    if (type !== 'fix') {
                        return [true];
                    }

                    // Combine body and footer so it matches no matter where it is placed
                    const fullContent = [body, footer].filter(Boolean).join('\n');

                    const regex = /^Fixes:\s+[0-9a-f]{7,40}\s+\(".+"\)/m;

                    if (regex.test(fullContent)) {
                        return [true];
                    }

                    return [
                        false,
                        'fix commits must reference a commit in the body/footer using the format: Fixes: <hash> ("<description>")',
                    ];
                },
            }
        },
    ]
};
